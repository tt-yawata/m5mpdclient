/*
    Name:      M5StackIRdecode.h
    Created:    2018/12/10
    Author:     tt.yawata@gmail.com
*/
#ifndef _M5StackIRdecpde_h
#define _M5StackIRdecode_h

#include <M5Stack.h>
#include "driver/rmt.h"
#include "driver/periph_ctrl.h"
#include "soc/rmt_reg.h"

#define NEC_TIC           560
#define NEC_LEADER_HIGH   NEC_TIC*16
#define NEC_LEADER_LOW    NEC_TIC*8
#define NEC_LEADER_LOWR   NEC_TIC*4
#define NEC_MARGIN        100
#define IGNORE_REPEAT

#define KDN_TIC           400
#define KDN_LEADER_HIGH   KDN_TIC*8
#define KDN_LEADER_LOW    KDN_TIC*4
#define KDN_MARGIN        100

#define SNY_TIC           600
#define SNY_LEADER_HIGH   SNY_TIC*4
#define SNY_MARGIN        100

#define UNI_TIC           560
#define UNI_LEADER_HIGH   UNI_TIC*4
#define UNI_LEADER_LOW    UNI_TIC*2
#define UNI_MARGIN        100

#define MB_TIC           300
#define MB_LEADER_HIGH   MB_TIC
#define MB_LEADER_LOW    MB_TIC*2
#define MB_MARGIN        100

#define TIC_MAX          (NEC_LEADER_HIGH*2)
#define TIC_MIN          (MB_TIC/2)

class M5StackIRdecode {
  public:
    M5StackIRdecode();
    boolean parseData(rmt_item32_t *item, size_t rxSize);
    String _ircode;
  private:
    String checkLeader(rmt_item32_t item);
    int16_t decodeBit(int16_t d0, int16_t d1, boolean dm);
    String rmttmp;
};

#endif
