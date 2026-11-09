/*
 * ma_config.h - which parts of miniaudio the host uses (stage 123). Included
 * before miniaudio.h by both the file that compiles it (miniaudio_impl.c)
 * and the one that calls it (device.c): the two must agree, as the switches
 * change miniaudio's structures.
 */
#ifndef RBH_MA_CONFIG_H
#define RBH_MA_CONFIG_H

#define MA_NO_ENCODING
#define MA_NO_DECODING
#define MA_NO_GENERATION
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MA_NO_ENGINE

#endif /* RBH_MA_CONFIG_H */
