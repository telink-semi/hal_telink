/******************************************************************************
 * Copyright (c) 2023-2024 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************/

#include "b9x_rf_power.h"
#if CONFIG_SOC_RISCV_TELINK_B91 || CONFIG_SOC_RISCV_TELINK_B92
 #include <rf.h>
#elif CONFIG_SOC_RISCV_TELINK_B95 || CONFIG_SOC_RISCV_TELINK_TL321X
#include <rf_common.h>
#endif

#if CONFIG_SOC_RISCV_TELINK_B91
/* TX power B91 lookup table */
const uint8_t b9x_tx_pwr_lt[] = {
	RF_POWER_N30dBm,        /* -30.0 dBm: -30 */
	RF_POWER_N30dBm,        /* -30.0 dBm: -29 */
	RF_POWER_N30dBm,        /* -30.0 dBm: -28 */
	RF_POWER_N30dBm,        /* -30.0 dBm: -27 */
	RF_POWER_N30dBm,        /* -30.0 dBm: -26 */
	RF_POWER_N23p54dBm,     /* -23.5 dBm: -25 */
	RF_POWER_N23p54dBm,     /* -23.5 dBm: -24 */
	RF_POWER_N23p54dBm,     /* -23.5 dBm: -23 */
	RF_POWER_N23p54dBm,     /* -23.5 dBm: -22 */
	RF_POWER_N23p54dBm,     /* -23.5 dBm: -21 */
	RF_POWER_N17p83dBm,     /* -17.8 dBm: -20 */
	RF_POWER_N17p83dBm,     /* -17.8 dBm: -19 */
	RF_POWER_N17p83dBm,     /* -17.8 dBm: -18 */
	RF_POWER_N17p83dBm,     /* -17.8 dBm: -17 */
	RF_POWER_N17p83dBm,     /* -17.8 dBm: -16 */
	RF_POWER_N12p06dBm,     /* -12.0 dBm: -15 */
	RF_POWER_N12p06dBm,     /* -12.0 dBm: -14 */
	RF_POWER_N12p06dBm,     /* -12.0 dBm: -13 */
	RF_POWER_N12p06dBm,     /* -12.0 dBm: -12 */
	RF_POWER_N12p06dBm,     /* -12.0 dBm: -11 */
	RF_POWER_N8p78dBm,      /*  -8.7 dBm: -10 */
	RF_POWER_N8p78dBm,      /*  -8.7 dBm:  -9 */
	RF_POWER_N8p78dBm,      /*  -8.7 dBm:  -8 */
	RF_POWER_N6p54dBm,      /*  -6.5 dBm:  -7 */
	RF_POWER_N6p54dBm,      /*  -6.5 dBm:  -6 */
	RF_POWER_N4p77dBm,      /*  -4.7 dBm:  -5 */
	RF_POWER_N4p77dBm,      /*  -4.7 dBm:  -4 */
	RF_POWER_N3p37dBm,      /*  -3.3 dBm:  -3 */
	RF_POWER_N2p01dBm,      /*  -2.0 dBm:  -2 */
	RF_POWER_N1p37dBm,      /*  -1.3 dBm:  -1 */
	RF_POWER_P0p01dBm,      /*   0.0 dBm:   0 */
	RF_POWER_P0p80dBm,      /*   0.8 dBm:   1 */
	RF_POWER_P2p32dBm,      /*   2.3 dBm:   2 */
	RF_POWER_P3p25dBm,      /*   3.2 dBm:   3 */
	RF_POWER_P4p35dBm,      /*   4.3 dBm:   4 */
	RF_POWER_P5p68dBm,      /*   5.6 dBm:   5 */
	RF_POWER_P5p68dBm,      /*   5.6 dBm:   6 */
	RF_POWER_P6p98dBm,      /*   6.9 dBm:   7 */
	RF_POWER_P8p05dBm,      /*   8.0 dBm:   8 */
	RF_POWER_P9p11dBm,      /*   9.1 dBm:   9 */
};

#elif CONFIG_SOC_RISCV_TELINK_B92 || CONFIG_SOC_RISCV_TELINK_B95
/* TX power B92 lookup table */
const uint8_t b9x_tx_pwr_lt[] = {
	RF_POWER_N30dBm,        /* -30.0 dBm: -30 */
	RF_POWER_N30dBm,        /* -30.0 dBm: -29 */
	RF_POWER_N30dBm,        /* -30.0 dBm: -28 */
	RF_POWER_N30dBm,        /* -30.0 dBm: -27 */
	RF_POWER_N30dBm,        /* -30.0 dBm: -26 */
	RF_POWER_N22p53dBm,     /* -22.5 dBm: -25 */
	RF_POWER_N22p53dBm,     /* -22.5 dBm: -24 */
	RF_POWER_N22p53dBm,     /* -22.5 dBm: -23 */
	RF_POWER_N22p53dBm,     /* -22.5 dBm: -22 */
	RF_POWER_N22p53dBm,     /* -22.5 dBm: -21 */
	RF_POWER_N13p42dBm,     /* -13.5 dBm: -20 */
	RF_POWER_N13p42dBm,     /* -13.5 dBm: -19 */
	RF_POWER_N13p42dBm,     /* -13.5 dBm: -18 */
	RF_POWER_N13p42dBm,     /* -13.5 dBm: -17 */
	RF_POWER_N13p42dBm,     /* -13.5 dBm: -16 */
	RF_POWER_N9p03dBm,      /* -9.0 dBm: -15 */
	RF_POWER_N9p03dBm,      /* -9.0 dBm: -14 */
	RF_POWER_N9p03dBm,      /* -9.0 dBm: -13 */
	RF_POWER_N9p03dBm,      /* -9.0 dBm: -12 */
	RF_POWER_N9p03dBm,      /* -9.0 dBm: -11 */
	RF_POWER_N5p94dBm,      /*  -5.9 dBm: -10 */
	RF_POWER_N5p94dBm,      /*  -5.9 dBm:  -9 */
	RF_POWER_N5p94dBm,      /*  -5.9 dBm:  -8 */
	RF_POWER_N5p94dBm,      /*  -5.9 dBm:  -7 */
	RF_POWER_N5p94dBm,      /*  -5.9 dBm:  -6 */
	RF_POWER_N3p95dBm,      /*  -4.0 dBm:  -5 */
	RF_POWER_N3p95dBm,      /*  -4.0 dBm:  -4 */
	RF_POWER_N2p51dBm,      /*  -2.5 dBm:  -3 */
	RF_POWER_N1p52dBm,      /*  -1.5 dBm:  -2 */
	RF_POWER_N0p43dBm,      /*  -0.5 dBm:  -1 */
	RF_POWER_P0p01dBm,      /*   0.0 dBm:   0 */
	RF_POWER_P1p03dBm,      /*   1.0 dBm:   1 */
	RF_POWER_P1p62dBm,      /*   1.6 dBm:   2 */
	RF_POWER_P2p51dBm,      /*   2.5 dBm:   3 */
	RF_POWER_P3p51dBm,      /*   3.5 dBm:   4 */
	RF_POWER_P4p61dBm,      /*   4.6 dBm:   5 */
	RF_POWER_P5p21dBm,      /*   5.2 dBm:   6 */
	RF_POWER_P7p00dBm,      /*   7.0 dBm:   7 */
	RF_POWER_P8p25dBm,      /*   8.2 dBm:   8 */
	RF_POWER_P9p90dBm,      /*   9.9 dBm:   9 */
};

#elif CONFIG_SOC_RISCV_TELINK_TL321X
/* TX power TL321X lookup table */
const uint8_t tlx_tx_pwr_lt[] = {
	RF_POWER_N48p75dBm,   /**< -48.8 dbm: -19 */
	RF_POWER_N25p53dBm,   /**< -25.5 dbm: -18 */
	RF_POWER_N20p01dBm,   /**< -20.0 dbm: -17 */
	RF_POWER_N14p13dBm,   /**< -14.1 dbm: -16 */
	RF_POWER_N10p77dBm,   /**< -10.7 dbm: -15 */
	RF_POWER_N9p45dBm,    /**<  -9.5 dbm: -14 */
	RF_POWER_N8p32dBm,    /**<  -8.3 dbm: -13 */
	RF_POWER_N7p33dBm,    /**<  -7.3 dbm: -12 */
	RF_POWER_N6p53dBm,    /**<  -6.5 dbm: -11 */
	RF_POWER_N5p73dBm,    /**<  -5.7 dbm: -10 */
	RF_POWER_N5p07dBm,    /**<  -5.0 dbm:  -9 */
	RF_POWER_N4p41dBm,    /**<  -4.4 dbm:  -8 */
	RF_POWER_N3p21dBm,    /**<  -3.2 dbm:  -7 */
	RF_POWER_N2p80dBm,    /**<  -2.8 dbm:  -6 */
	RF_POWER_N2p21dBm,    /**<  -2.1 dbm:  -5 */
	RF_POWER_N1p49dBm,    /**<  -1.5 dbm:  -4 */
	RF_POWER_N1p10dBm,    /**<  -1.0 dbm:  -3 */
	RF_POWER_N0p72dBm,    /**<  -0.7 dbm:  -2 */
	RF_POWER_N0p41dBm,    /**<  -0.4 dbm:  -1 */
	RF_POWER_N0p07dBm,    /**<   0.0 dbm:   0 */
	RF_POWER_P0p25dBm,    /**<   0.2 dbm:   1 */
	RF_POWER_P0p55dBm,    /**<   0.5 dbm:   2 */
	RF_POWER_P1p09dBm,    /**<   1.0 dbm:   3 */
	RF_POWER_P1p33dBm,    /**<   1.3 dbm:   4 */
	RF_POWER_P1p53dBm,    /**<   1.5 dbm:   5 */
	RF_POWER_P2p03dBm,    /**<   2.0 dbm:   6 */
	RF_POWER_P2p45dBm,    /**<   2.5 dbm:   7 */
	RF_POWER_P2p99dBm,    /**<   3.0 dbm:   8 */
	RF_POWER_P3p49dBm,    /**<   3.5 dbm:   9 */
	RF_POWER_P4p05dBm,    /**<   4.0 dbm:  10 */
	RF_POWER_P4p52dBm,    /**<   4.5 dbm:  11 */
	RF_POWER_P4p73dBm,    /**<   4.7 dbm:  12 */
	RF_POWER_P5p00dBm,    /**<   5.0 dbm:  13 */
	RF_POWER_P5p09dBm,    /**<   5.1 dbm:  14 */
	RF_POWER_P5p25dBm,    /**<   5.2 dbm:  15 */
	RF_POWER_P5p54dBm,    /**<   5.5 dbm:  16 */
	RF_POWER_P5p79dBm,    /**<   5.8 dbm:  17 */
	RF_POWER_P5p97dBm,    /**<   6.0 dbm:  18 */
	RF_POWER_P6p48dBm,    /**<   6.5 dbm:  19 */
	RF_POWER_P6p97dBm,    /**<   7.0 dbm:  20 */
	RF_POWER_P7p19dBm,    /**<   7.2 dbm:  21 */
	RF_POWER_P7p61dBm,    /**<   7.6 dbm:  22 */
	RF_POWER_P8p03dBm,    /**<   8.0 dbm:  23 */
	RF_POWER_P8p48dBm,    /**<   8.5 dbm:  24 */
	RF_POWER_P8p95dBm,    /**<   9.0 dbm:  25 */
	RF_POWER_P9p50dBm,    /**<   9.5 dbm:  26 */
	RF_POWER_P9p97dBm,    /**<  10.0 dbm:  27 */
	RF_POWER_P10p56dBm,   /**<  10.5 dbm:  28 */
	RF_POWER_P10p78dBm,   /**<  10.8 dbm:  29 */
	RF_POWER_P11p16dBm,   /**<  11.1 dbm:  30 */
	RF_POWER_P11p33dBm    /**<  11.3 dbm:  31 */
};

#endif
