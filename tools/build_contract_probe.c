/* Native-only Stage 037 schema proof; not a new production binding/compiler.
 * Export complete descriptions in SI; import all fields before one validation
 * and one initialization. Geometry observations reuse the existing public API. */
#include "rb_internal.h"
#include "presets.h"
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEG (3.14159265358979323846/180.0)
#define RPM (3.14159265358979323846/30.0)
enum { S, B, C };
typedef struct { const char *name, *unit; size_t offset; int integer, shape; double scale; } field;
#define F(name,member,integer,shape,unit,scale) {#name,unit,offsetof(rb_engine_desc,member),integer,shape,scale},
static const field FIELDS[]={
#include "build_contract_fields.inc"
};
#undef F
#define COUNT (sizeof FIELDS/sizeof FIELDS[0])

static int count(const rb_engine_desc *d, const field *f)
{ return f->shape==B ? d->banks:f->shape==C ? d->cylinders:1; }

static double value(const rb_engine_desc *d,const field *f,int i)
{
    const char *p=(const char *)d+f->offset;
    return (f->integer ? ((const int *)p)[i]:((const double *)p)[i])*f->scale;
}

static int import(rb_engine_desc *d)
{
    unsigned char seen[COUNT][RB_MAX_CYLINDERS]={0};
    memset(d,0,sizeof *d);
    char line[256],name[100],extra;
    int index;
    double v;
    while (fgets(line,sizeof line,stdin)) {
        if (sscanf(line,"%99s %d %lf %c",name,&index,&v,&extra)!=3 || !isfinite(v)) return 1;
        size_t k;
        for(k=0;k<COUNT;k++) if(!strcmp(name,FIELDS[k].name)) break;
        if(k==COUNT) return 1;
        const field *f=&FIELDS[k];
        const int cap=f->shape==B ? 2:f->shape==C ? RB_MAX_CYLINDERS:1;
        if(index<0 || index>=cap || seen[k][index]++) return 1;
        v/=f->scale;
        if (!isfinite(v)) return 1;
        char *p=(char *)d+f->offset;
        if(f->integer) {
            if(v!=floor(v) || v < -1000000 || v > 1000000) return 1;
            ((int *)p)[index]=(int)v;
        } else ((double *)p)[index]=v;
    }
    if (ferror(stdin) || d->banks<1 || d->banks>2 || d->cylinders<1 ||
        d->cylinders>RB_MAX_CYLINDERS) return 1;
    for(size_t k=0;k<COUNT;k++)
        for(int i=0;i<RB_MAX_CYLINDERS;i++)
            if(seen[k][i] != (i<count(d,&FIELDS[k]))) return 1;
    return rb_engine_desc_check(d)!=0;
}

static int output(rb_sim *sim,const char *preset)
{
    const rb_engine_desc *d=&sim->engine.desc;
    printf("{\"schema_version\":1,\"preset\":\"%s\",\"parameters\":{",preset);
    for(size_t k=0;k<COUNT;k++) {
        const field *f=&FIELDS[k];
        printf("%s\"%s\":[",k ? ",":"",f->name);
        for(int i=0;i<count(d,f);i++) printf("%s%.17g",i ? ",":"",value(d,f,i));
        putchar(']');
    }
    printf("},\"catalog\":{");
    for(size_t k=0;k<COUNT;k++) {
        const field *f=&FIELDS[k];
        printf("%s\"%s\":{\"unit\":\"%s\",\"integer\":%s,\"shape\":\"%s\"}",
            k ? ",":"",f->name,f->unit,f->integer ? "true":"false",
            f->shape==B ? "bank":f->shape==C ? "cylinder":"scalar");
    }
    printf("},\"pipes\":[");
    for(int k=0;k<rb_pipe_count(sim);k++) {
        printf("%s{",k ? ",":"");
        for(int f=0;f<rb_pipe_field_count();f++)
            printf("%s\"%s\":%.17g",f ? ",":"",rb_pipe_field_name(f),rb_get_pipe(sim,k,f));
        putchar('}');
    }
    printf("],\"geometry_inventory\":{\"collectors\":[");
    for(int c=0;c<sim->engine.collectors;c++)
        printf("%s%.17g",c ? ",":"",sim->engine.collector[c].V);
    printf("],\"junction_volume_m3\":%.17g,\"pipes\":[",d->junction_volume);
    for(int k=0;k<rb_pipe_count(sim);k++) {
        const rb_pipe *p=rb_engine_pipe(&sim->engine,k);
        double volume=0;
        for(int j=0;j<p->cells;j++) volume+=p->cell[j].V;
        const rb_face *packed=p->cells>1 ? &p->face[0]:&p->end_face[0];
        printf("%s{\"kind\":%d,\"number\":%d,\"length_m\":%.17g,\"area_m2\":%.17g,\"volume_m3\":%.17g,\"packing_R\":%.17g,\"packing_tau\":%.17g,\"end_laminar\":[%.17g,%.17g],\"end_exit\":[%.17g,%.17g],\"friction_factor\":%.17g,\"wall_temperature_K\":%.17g}",
            k ? ",":"",(int)rb_get_pipe(sim,k,RB_PIPE_KIND),(int)rb_get_pipe(sim,k,RB_PIPE_NUMBER),
            p->length,p->area,volume,packed->packing,packed->settle_time,
            p->end_face[0].laminar,p->end_face[1].laminar,p->end_face[0].exit,p->end_face[1].exit,
            p->friction_factor,p->wall_temperature);
    }
    puts("]}}");
    return ferror(stdout)!=0;
}

int main(int argc,char **argv)
{
    if(argc<3 || argc>4 || (strcmp(argv[1],"export") && strcmp(argv[1],"import"))) return 2;
    int preset=-1;
    for(int i=0;i<rb_preset_count();i++) if(!strcmp(argv[2],rb_preset_key(i))) preset=i;
    if(preset<0) return 2;
    rb_engine_desc d;
    if(!strcmp(argv[1],"import")) {
        if(argc!=3 || import(&d)) return 3;
    } else {
        if(rb_preset_desc(preset,&d)) return 3;
        if(argc==4) {
            if(strcmp(argv[3],"--no-runners")) return 2;
            d.runner_length=0;
        }
    }
    rb_sim *sim=rb_create_with(48000,4);
    if(!sim) return 4;
    const int status=rb_engine_init_at(&sim->engine,&d,1.0/192000,48000);
    const int result=status ? 4:output(sim,rb_preset_key(preset));
    rb_destroy(sim);
    return result;
}
