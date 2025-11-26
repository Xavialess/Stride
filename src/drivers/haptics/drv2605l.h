/*
 * Copyright (c) 2024 Xavier Quintanilla
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DRV2605L_H
#define DRV2605L_H

#include <zephyr/types.h>
#include <zephyr/device.h>

/**
 * @file drv2605l.h
 * @brief Driver for DRV2605L Haptic Motor Driver
 * 
 * The DRV2605L is an I2C haptic driver with 123 built-in waveform effects.
 * Designed for ERM (Eccentric Rotating Mass) and LRA (Linear Resonant Actuator) motors.
 */

/* DRV2605L Register Addresses */
#define DRV2605L_REG_STATUS          0x00
#define DRV2605L_REG_MODE            0x01
#define DRV2605L_REG_RTPIN           0x02
#define DRV2605L_REG_LIBRARY         0x03
#define DRV2605L_REG_WAVESEQ1        0x04
#define DRV2605L_REG_WAVESEQ2        0x05
#define DRV2605L_REG_WAVESEQ3        0x06
#define DRV2605L_REG_WAVESEQ4        0x07
#define DRV2605L_REG_WAVESEQ5        0x08
#define DRV2605L_REG_WAVESEQ6        0x09
#define DRV2605L_REG_WAVESEQ7        0x0A
#define DRV2605L_REG_WAVESEQ8        0x0B
#define DRV2605L_REG_GO              0x0C
#define DRV2605L_REG_OVERDRIVE       0x0D
#define DRV2605L_REG_SUSTAINPOS      0x0E
#define DRV2605L_REG_SUSTAINNEG      0x0F
#define DRV2605L_REG_BREAK           0x10
#define DRV2605L_REG_AUDIOCTRL       0x11
#define DRV2605L_REG_AUDIOLVL        0x12
#define DRV2605L_REG_AUDIOMAX        0x13
#define DRV2605L_REG_RATEDV          0x16
#define DRV2605L_REG_CLAMPV          0x17
#define DRV2605L_REG_AUTOCALCOMP     0x18
#define DRV2605L_REG_AUTOCALEMP      0x19
#define DRV2605L_REG_FEEDBACK        0x1A
#define DRV2605L_REG_CONTROL1        0x1B
#define DRV2605L_REG_CONTROL2        0x1C
#define DRV2605L_REG_CONTROL3        0x1D
#define DRV2605L_REG_CONTROL4        0x1E
#define DRV2605L_REG_VBAT            0x21
#define DRV2605L_REG_LRARESON        0x22

/* Mode Register Values */
#define DRV2605L_MODE_INTTRIG        0x00  /* Internal trigger */
#define DRV2605L_MODE_EXTTRIGEDGE    0x01  /* External edge trigger */
#define DRV2605L_MODE_EXTTRIGLVL     0x02  /* External level trigger */
#define DRV2605L_MODE_PWMANALOG      0x03  /* PWM/Analog input */
#define DRV2605L_MODE_AUDIOVIBE      0x04  /* Audio-to-vibe */
#define DRV2605L_MODE_REALTIME       0x05  /* Real-time playback */
#define DRV2605L_MODE_DIAGNOS        0x06  /* Diagnostics */
#define DRV2605L_MODE_AUTOCAL        0x07  /* Auto calibration */
#define DRV2605L_MODE_STANDBY        0x40  /* Standby mode */
#define DRV2605L_MODE_RESET          0x80  /* Software reset */

/* Library Selection */
#define DRV2605L_LIB_EMPTY           0x00  /* Empty */
#define DRV2605L_LIB_ERM             0x01  /* ERM library A */
#define DRV2605L_LIB_ERM_B           0x02  /* ERM library B */
#define DRV2605L_LIB_ERM_C           0x03  /* ERM library C */
#define DRV2605L_LIB_ERM_D           0x04  /* ERM library D */
#define DRV2605L_LIB_ERM_E           0x05  /* ERM library E */
#define DRV2605L_LIB_LRA             0x06  /* LRA library */
#define DRV2605L_LIB_ERM_F           0x07  /* ERM library F */

/* Waveform Effects (1-123) */
#define DRV2605L_EFFECT_STRONG_CLICK_100      1
#define DRV2605L_EFFECT_STRONG_CLICK_60       2
#define DRV2605L_EFFECT_STRONG_CLICK_30       3
#define DRV2605L_EFFECT_SHARP_CLICK_100       4
#define DRV2605L_EFFECT_SHARP_CLICK_60        5
#define DRV2605L_EFFECT_SHARP_CLICK_30        6
#define DRV2605L_EFFECT_SOFT_BUMP_100         7
#define DRV2605L_EFFECT_SOFT_BUMP_60          8
#define DRV2605L_EFFECT_SOFT_BUMP_30          9
#define DRV2605L_EFFECT_DOUBLE_CLICK_100      10
#define DRV2605L_EFFECT_DOUBLE_CLICK_60       11
#define DRV2605L_EFFECT_TRIPLE_CLICK_100      12
#define DRV2605L_EFFECT_SOFT_FUZZ_60          13
#define DRV2605L_EFFECT_STRONG_BUZZ_100       14
#define DRV2605L_EFFECT_ALERT_750MS           15
#define DRV2605L_EFFECT_ALERT_1000MS          16
#define DRV2605L_EFFECT_STRONG_CLICK_1        17
#define DRV2605L_EFFECT_STRONG_CLICK_2_75     18
#define DRV2605L_EFFECT_STRONG_CLICK_3_60     19
#define DRV2605L_EFFECT_STRONG_CLICK_4_30     20
#define DRV2605L_EFFECT_MEDIUM_CLICK_1        21
#define DRV2605L_EFFECT_MEDIUM_CLICK_2        22
#define DRV2605L_EFFECT_MEDIUM_CLICK_3        23
#define DRV2605L_EFFECT_SHARP_TICK_1          24
#define DRV2605L_EFFECT_SHARP_TICK_2          25
#define DRV2605L_EFFECT_SHARP_TICK_3          26
#define DRV2605L_EFFECT_SHORT_DOUBLE_CLICK_STRONG_1  27
#define DRV2605L_EFFECT_SHORT_DOUBLE_CLICK_STRONG_2  28
#define DRV2605L_EFFECT_SHORT_DOUBLE_CLICK_STRONG_3  29
#define DRV2605L_EFFECT_SHORT_DOUBLE_CLICK_STRONG_4  30
#define DRV2605L_EFFECT_SHORT_DOUBLE_CLICK_MEDIUM_1  31
#define DRV2605L_EFFECT_SHORT_DOUBLE_CLICK_MEDIUM_2  32
#define DRV2605L_EFFECT_SHORT_DOUBLE_CLICK_MEDIUM_3  33
#define DRV2605L_EFFECT_SHORT_DOUBLE_SHARP_TICK_1    34
#define DRV2605L_EFFECT_SHORT_DOUBLE_SHARP_TICK_2    35
#define DRV2605L_EFFECT_SHORT_DOUBLE_SHARP_TICK_3    36
#define DRV2605L_EFFECT_LONG_DOUBLE_SHARP_CLICK_STRONG_1  37
#define DRV2605L_EFFECT_LONG_DOUBLE_SHARP_CLICK_STRONG_2  38
#define DRV2605L_EFFECT_LONG_DOUBLE_SHARP_CLICK_STRONG_3  39
#define DRV2605L_EFFECT_LONG_DOUBLE_SHARP_CLICK_STRONG_4  40
#define DRV2605L_EFFECT_LONG_DOUBLE_SHARP_CLICK_MEDIUM_1  41
#define DRV2605L_EFFECT_LONG_DOUBLE_SHARP_CLICK_MEDIUM_2  42
#define DRV2605L_EFFECT_LONG_DOUBLE_SHARP_CLICK_MEDIUM_3  43
#define DRV2605L_EFFECT_LONG_DOUBLE_SHARP_TICK_1     44
#define DRV2605L_EFFECT_LONG_DOUBLE_SHARP_TICK_2     45
#define DRV2605L_EFFECT_LONG_DOUBLE_SHARP_TICK_3     46
#define DRV2605L_EFFECT_BUZZ_1                47
#define DRV2605L_EFFECT_BUZZ_2                48
#define DRV2605L_EFFECT_BUZZ_3                49
#define DRV2605L_EFFECT_BUZZ_4                50
#define DRV2605L_EFFECT_BUZZ_5                51
#define DRV2605L_EFFECT_PULSING_STRONG_1      52
#define DRV2605L_EFFECT_PULSING_STRONG_2      53
#define DRV2605L_EFFECT_PULSING_MEDIUM_1      54
#define DRV2605L_EFFECT_PULSING_MEDIUM_2      55
#define DRV2605L_EFFECT_PULSING_MEDIUM_3      56
#define DRV2605L_EFFECT_PULSING_SHARP_1       57
#define DRV2605L_EFFECT_PULSING_SHARP_2       58
#define DRV2605L_EFFECT_TRANSITION_CLICK_1    59
#define DRV2605L_EFFECT_TRANSITION_CLICK_2    60
#define DRV2605L_EFFECT_TRANSITION_CLICK_3    61
#define DRV2605L_EFFECT_TRANSITION_CLICK_4    62
#define DRV2605L_EFFECT_TRANSITION_CLICK_5    63
#define DRV2605L_EFFECT_TRANSITION_CLICK_6    64
#define DRV2605L_EFFECT_TRANSITION_HUM_1      65
#define DRV2605L_EFFECT_TRANSITION_HUM_2      66
#define DRV2605L_EFFECT_TRANSITION_HUM_3      67
#define DRV2605L_EFFECT_TRANSITION_HUM_4      68
#define DRV2605L_EFFECT_TRANSITION_HUM_5      69
#define DRV2605L_EFFECT_TRANSITION_HUM_6      70
#define DRV2605L_EFFECT_TRANSITION_RAMP_DOWN_LONG_SMOOTH_1  71
#define DRV2605L_EFFECT_TRANSITION_RAMP_DOWN_LONG_SMOOTH_2  72
#define DRV2605L_EFFECT_TRANSITION_RAMP_DOWN_MEDIUM_SMOOTH_1  73
#define DRV2605L_EFFECT_TRANSITION_RAMP_DOWN_MEDIUM_SMOOTH_2  74
#define DRV2605L_EFFECT_TRANSITION_RAMP_DOWN_SHORT_SMOOTH_1  75
#define DRV2605L_EFFECT_TRANSITION_RAMP_DOWN_SHORT_SMOOTH_2  76
#define DRV2605L_EFFECT_TRANSITION_RAMP_DOWN_LONG_SHARP_1  77
#define DRV2605L_EFFECT_TRANSITION_RAMP_DOWN_LONG_SHARP_2  78
#define DRV2605L_EFFECT_TRANSITION_RAMP_DOWN_MEDIUM_SHARP_1  79
#define DRV2605L_EFFECT_TRANSITION_RAMP_DOWN_MEDIUM_SHARP_2  80
#define DRV2605L_EFFECT_TRANSITION_RAMP_DOWN_SHORT_SHARP_1  81
#define DRV2605L_EFFECT_TRANSITION_RAMP_DOWN_SHORT_SHARP_2  82
#define DRV2605L_EFFECT_TRANSITION_RAMP_UP_LONG_SMOOTH_1  83
#define DRV2605L_EFFECT_TRANSITION_RAMP_UP_LONG_SMOOTH_2  84
#define DRV2605L_EFFECT_TRANSITION_RAMP_UP_MEDIUM_SMOOTH_1  85
#define DRV2605L_EFFECT_TRANSITION_RAMP_UP_MEDIUM_SMOOTH_2  86
#define DRV2605L_EFFECT_TRANSITION_RAMP_UP_SHORT_SMOOTH_1  87
#define DRV2605L_EFFECT_TRANSITION_RAMP_UP_SHORT_SMOOTH_2  88
#define DRV2605L_EFFECT_TRANSITION_RAMP_UP_LONG_SHARP_1  89
#define DRV2605L_EFFECT_TRANSITION_RAMP_UP_LONG_SHARP_2  90
#define DRV2605L_EFFECT_TRANSITION_RAMP_UP_MEDIUM_SHARP_1  91
#define DRV2605L_EFFECT_TRANSITION_RAMP_UP_MEDIUM_SHARP_2  92
#define DRV2605L_EFFECT_TRANSITION_RAMP_UP_SHORT_SHARP_1  93
#define DRV2605L_EFFECT_TRANSITION_RAMP_UP_SHORT_SHARP_2  94
#define DRV2605L_EFFECT_LONG_BUZZ_PROGRAMMATIC  95
#define DRV2605L_EFFECT_SMOOTH_HUM_1          96
#define DRV2605L_EFFECT_SMOOTH_HUM_2          97
#define DRV2605L_EFFECT_SMOOTH_HUM_3          98
#define DRV2605L_EFFECT_SMOOTH_HUM_4          99
#define DRV2605L_EFFECT_SMOOTH_HUM_5          100
#define DRV2605L_EFFECT_ALERT_1000MS_2        101
#define DRV2605L_EFFECT_ALERT_750MS_2         102
#define DRV2605L_EFFECT_ALERT_500MS           103
#define DRV2605L_EFFECT_ALERT_250MS           104
#define DRV2605L_EFFECT_PULSING_STRONG_3      105
#define DRV2605L_EFFECT_PULSING_MEDIUM_4      106
#define DRV2605L_EFFECT_PULSING_MEDIUM_5      107
#define DRV2605L_EFFECT_PULSING_SHARP_3       108
#define DRV2605L_EFFECT_PULSING_SHARP_4       109
#define DRV2605L_EFFECT_PULSING_SHARP_5       110
#define DRV2605L_EFFECT_LONG_BUZZ_1           111
#define DRV2605L_EFFECT_LONG_BUZZ_2           112
#define DRV2605L_EFFECT_LONG_BUZZ_3           113
#define DRV2605L_EFFECT_LONG_BUZZ_4           114
#define DRV2605L_EFFECT_SMOOTH_HUM_6          115
#define DRV2605L_EFFECT_SMOOTH_HUM_7          116
#define DRV2605L_EFFECT_SMOOTH_HUM_8          117
#define DRV2605L_EFFECT_SMOOTH_HUM_9          118
#define DRV2605L_EFFECT_SMOOTH_HUM_10         119
#define DRV2605L_EFFECT_SMOOTH_HUM_11         120
#define DRV2605L_EFFECT_SMOOTH_HUM_12         121
#define DRV2605L_EFFECT_SMOOTH_HUM_13         122
#define DRV2605L_EFFECT_SMOOTH_HUM_14         123

/* Maximum waveform sequence length */
#define DRV2605L_MAX_WAVEFORM_SEQ     8

/* Motor type */
typedef enum {
	DRV2605L_MOTOR_ERM,  /* Eccentric Rotating Mass */
	DRV2605L_MOTOR_LRA   /* Linear Resonant Actuator */
} drv2605l_motor_type_t;

/**
 * @brief Initialize DRV2605L haptic driver
 * 
 * @param motor_type Type of motor connected (ERM or LRA)
 * @return 0 on success, negative errno on failure
 */
int drv2605l_init(drv2605l_motor_type_t motor_type);

/**
 * @brief Play a single haptic effect
 * 
 * @param effect Effect number (1-123)
 * @return 0 on success, negative errno on failure
 */
int drv2605l_play_effect(uint8_t effect);

/**
 * @brief Play a sequence of haptic effects
 * 
 * @param effects Array of effect numbers (1-123, 0 terminates)
 * @param count Number of effects in sequence (max 8)
 * @return 0 on success, negative errno on failure
 */
int drv2605l_play_sequence(const uint8_t *effects, uint8_t count);

/**
 * @brief Stop haptic playback
 * 
 * @return 0 on success, negative errno on failure
 */
int drv2605l_stop(void);

/**
 * @brief Set motor to standby mode (low power)
 * 
 * @return 0 on success, negative errno on failure
 */
int drv2605l_standby(void);

/**
 * @brief Wake motor from standby
 * 
 * @return 0 on success, negative errno on failure
 */
int drv2605l_wakeup(void);

/**
 * @brief Check if motor is currently playing
 * 
 * @return true if playing, false otherwise
 */
bool drv2605l_is_playing(void);

/**
 * @brief Perform auto-calibration (for LRA motors)
 * 
 * @return 0 on success, negative errno on failure
 */
int drv2605l_auto_calibrate(void);

#endif /* DRV2605L_H */

