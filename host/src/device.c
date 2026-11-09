/*
 * device.c - the audio device through miniaudio (stage 123): listing, choosing
 * and opening a playback device, the rates (44.1 and 48 kHz rendered natively,
 * any other fed 48 kHz and converted by miniaudio), and telling the host when
 * the device stops or is rerouted. The device's thread only calls
 * rb_host_pull: no physics runs on it.
 *
 * The null backend (miniaudio's) is a real device thread that pulls audio at
 * a real device's pace and plays it nowhere: what every test uses.
 */
#include <stdio.h>
#include <string.h>

#include "ma_config.h"
#include "miniaudio.h"

#include "host_internal.h"

typedef struct rbh_device rbh_device;

struct rbh_device {
    ma_context context;
    ma_device  device;
    int        context_ok, device_ok;
    rb_host   *host;
    atomic_int stopping;           /* we asked it to stop: not a lost device */
};

int rb_host_pick_rate(const int32_t *native_rates, int count)
{
    int has_48 = 0, has_44 = 0, any = 0;
    if (!native_rates || count <= 0) return 48000;
    for (int i = 0; i < count; i++) {
        if (native_rates[i] == 0) any = 1;
        else if (native_rates[i] == 48000) has_48 = 1;
        else if (native_rates[i] == 44100) has_44 = 1;
    }
    if (any || has_48) return 48000;
    if (has_44) return 44100;
    return 48000;       /* any other rate: render 48 kHz and let miniaudio convert */
}

/* A context on the backend the host asked for: the null one, or miniaudio's own
 * choice for the system. */
static int context_init(ma_context *context, int backend)
{
    if (backend == RB_HOST_BACKEND_NULL) {
        const ma_backend only_null[1] = { ma_backend_null };
        return ma_context_init(only_null, 1, NULL, context) == MA_SUCCESS;
    }
    return ma_context_init(NULL, 0, NULL, context) == MA_SUCCESS;
}

/* The device's native sample rates, as a list. */
static int native_rates(ma_context *context, const ma_device_id *id, int32_t *rates, int max)
{
    ma_device_info info;
    if (ma_context_get_device_info(context, ma_device_type_playback, id, &info) != MA_SUCCESS) return 0;
    int n = 0;
    for (ma_uint32 k = 0; k < info.nativeDataFormatCount && n < max; k++) rates[n++] = (int32_t)info.nativeDataFormats[k].sampleRate;
    return n;
}

int rbh_device_list(int backend, rb_host_device_info *out, int max)
{
    ma_context context;
    if (!context_init(&context, backend)) return RB_HOST_ERR_DEVICE;
    ma_device_info *devices = NULL;
    ma_uint32 count = 0;
    if (ma_context_get_devices(&context, &devices, &count, NULL, NULL) != MA_SUCCESS) {
        ma_context_uninit(&context);
        return RB_HOST_ERR_DEVICE;
    }
    for (ma_uint32 i = 0; i < count && (int)i < max; i++) {
        memset(&out[i], 0, sizeof out[i]);
        out[i].index = (int32_t)i;
        out[i].is_default = devices[i].isDefault ? 1 : 0;
        snprintf(out[i].name, sizeof out[i].name, "%s", devices[i].name);
        int32_t rates[8];
        const int n = native_rates(&context, &devices[i].id, rates, 8);
        out[i].native_rate = n > 0 ? rates[0] : 0;
    }
    ma_context_uninit(&context);
    return (int)count;
}

int rb_host_list_devices(int backend, rb_host_device_info *out, int max)
{
    if ((backend != RB_HOST_BACKEND_NULL && backend != RB_HOST_BACKEND_DEFAULT) || !out || max < 0) return RB_HOST_ERR_ARG;
    return rbh_device_list(backend, out, max);
}

int rbh_device_rate(int backend, int index)
{
    ma_context context;
    if (!context_init(&context, backend)) return 0;
    ma_device_info *devices = NULL;
    ma_uint32 count = 0;
    int rate = 0;
    if (ma_context_get_devices(&context, &devices, &count, NULL, NULL) == MA_SUCCESS && count > 0) {
        ma_uint32 chosen = 0;
        if (index >= 0) chosen = (ma_uint32)index;
        else
            for (ma_uint32 i = 0; i < count; i++)
                if (devices[i].isDefault) chosen = i;
        if (chosen < count) {
            int32_t rates[8];
            const int n = native_rates(&context, &devices[chosen].id, rates, 8);
            rate = rb_host_pick_rate(rates, n);
        }
    }
    ma_context_uninit(&context);
    return rate;
}

/* miniaudio's data callback, on the device's thread: the ring's frames out. */
static void data_callback(ma_device *device, void *output, const void *input, ma_uint32 frames)
{
    (void)input;
    rbh_device *d = (rbh_device *)device->pUserData;
    rb_host_pull(d->host, (float *)output, (int)frames);
}

static void notification_callback(const ma_device_notification *notification)
{
    rbh_device *d = (rbh_device *)notification->pDevice->pUserData;
    if (!d || !d->host) return;
    switch (notification->type) {
    case ma_device_notification_type_stopped:
        if (!atomic_load(&d->stopping)) rbh_host_device_event(d->host, 1);
        break;
    case ma_device_notification_type_rerouted:
        rbh_host_device_event(d->host, 2);
        break;
    default:
        break;
    }
}

int rbh_device_open(rb_host *h, int backend, int index, int rate, int period_ms, struct rbh_device **out)
{
    *out = NULL;
    rbh_device *d = (rbh_device *)rbh_alloc(sizeof *d);
    if (!d) return RB_HOST_ERR_NOMEM;
    d->host = h;
    atomic_init(&d->stopping, 0);
    if (!context_init(&d->context, backend)) {
        rbh_free(d);
        return RB_HOST_ERR_DEVICE;
    }
    d->context_ok = 1;
    ma_device_info *devices = NULL;
    ma_uint32 count = 0;
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = RB_HOST_CHANNELS;
    config.sampleRate = (ma_uint32)rate;
    config.periodSizeInMilliseconds = (ma_uint32)period_ms;
    config.performanceProfile = ma_performance_profile_low_latency;
    config.dataCallback = data_callback;
    config.notificationCallback = notification_callback;
    config.pUserData = d;
    if (index >= 0) {
        if (ma_context_get_devices(&d->context, &devices, &count, NULL, NULL) != MA_SUCCESS || (ma_uint32)index >= count) {
            rbh_device_close(d);
            return RB_HOST_ERR_DEVICE;
        }
        config.playback.pDeviceID = &devices[index].id;
    }
    if (ma_device_init(&d->context, &config, &d->device) != MA_SUCCESS) {
        rbh_device_close(d);
        return RB_HOST_ERR_DEVICE;
    }
    d->device_ok = 1;
    /* the device's own latency: its buffers, in our frames */
    const double internal_rate = d->device.playback.internalSampleRate ? (double)d->device.playback.internalSampleRate : (double)rate;
    const double frames = (double)d->device.playback.internalPeriodSizeInFrames * (double)d->device.playback.internalPeriods;
    rb_host_set_device_latency(h, (uint32_t)(frames * (double)rate / internal_rate + 0.5));
    *out = d;
    return RB_HOST_OK;
}

int rbh_device_start(struct rbh_device *d)
{
    if (!d || !d->device_ok) return RB_HOST_ERR_DEVICE;
    return ma_device_start(&d->device) == MA_SUCCESS ? RB_HOST_OK : RB_HOST_ERR_DEVICE;
}

void rbh_device_stop(struct rbh_device *d)
{
    if (!d || !d->device_ok) return;
    atomic_store(&d->stopping, 1);
    ma_device_stop(&d->device);      /* returns when the callback is no longer running */
    atomic_store(&d->stopping, 0);
}

void rbh_device_close(struct rbh_device *d)
{
    if (!d) return;
    if (d->device_ok) {
        atomic_store(&d->stopping, 1);
        ma_device_uninit(&d->device);
    }
    if (d->context_ok) ma_context_uninit(&d->context);
    rbh_free(d);
}
