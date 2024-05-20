#ifndef ANDROID_BOOTVIDEO_DMX_H
#define ANDROID_BOOTVIDEO_DMX_H

#define DMX_CLEAR_CACHE		0

typedef enum dmx_input_source {
    INPUT_DEMOD,
    INPUT_LOCAL,
    INPUT_LOCAL_SEC
} dmx_input_source_t;

enum {
    DMA_0 = 0,
    DMA_1,
    DMA_2,
    DMA_3,
    DMA_4,
    DMA_5,
    DMA_6,
    DMA_7,
    FRONTEND_TS0 = 32,
    FRONTEND_TS1,
    FRONTEND_TS2,
    FRONTEND_TS3,
    FRONTEND_TS4,
    FRONTEND_TS5,
    FRONTEND_TS6,
    FRONTEND_TS7,
    DMA_0_1 = 64,
    DMA_1_1,
    DMA_2_1,
    DMA_3_1,
    DMA_4_1,
    DMA_5_1,
    DMA_6_1,
    DMA_7_1,
    FRONTEND_TS0_1 = 96,
    FRONTEND_TS1_1,
    FRONTEND_TS2_1,
    FRONTEND_TS3_1,
    FRONTEND_TS4_1,
    FRONTEND_TS5_1,
    FRONTEND_TS6_1,
    FRONTEND_TS7_1,
};

struct dmx_set_command_info {
	__u32 command;
	__u32 reserved0;
	__u32 reserved1;
};

#define DMX_SET_INPUT           _IO('o', 80)
#define DMX_SET_HW_SOURCE       _IO('o', 82)
#define DMX_SET_COMMAND         _IOW('o', 89, struct dmx_set_command_info)

#endif //ANDROID_BOOTVIDEO_DMX_H

