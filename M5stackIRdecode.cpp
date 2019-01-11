/*
    Name:      M5StackIRdecode.cc
    Created:    2018/12/10
    Author:     tt.yawata@gmail.com
*/
#include "M5StackIRdecode.h"

M5StackIRdecode::M5StackIRdecode() {};

String M5StackIRdecode::checkLeader(rmt_item32_t item) {

  if ((item.level0 != 0) || (item.level1 != 1)) {
    return "";
  }
  if ( item.duration0 > (NEC_LEADER_HIGH - NEC_MARGIN)) {
    if ( item.duration1 > (NEC_LEADER_LOW - NEC_MARGIN)) {
      return "N"; //NEC
    } else if ( item.duration1 > (NEC_LEADER_LOWR - NEC_MARGIN)) {
#ifdef IGNORE_REPEAT
      return "";
#else
      return "R";
#endif
    } else {
      return "";
    }
  } else if ((item.duration0 > (KDN_LEADER_HIGH - KDN_MARGIN))
             && (item.duration1 > (KDN_LEADER_LOW - KDN_MARGIN ))) {
    return "K"; // KADEN
  } else if ( item.duration0 > (SNY_LEADER_HIGH - SNY_MARGIN)) {
    return "S"; // SONY
  } else if ((item.duration0 > (UNI_LEADER_HIGH - UNI_MARGIN))
             && (item.duration1 > (UNI_LEADER_LOW - UNI_MARGIN ))) {
    return "U"; //UNIDEN
  } else if (item.duration0 > (NEC_TIC - NEC_MARGIN)) { // Ignore Victor repeat
    return "" ;
  } else if ((item.duration0 > (MB_LEADER_HIGH - MB_MARGIN))
             && (item.duration1 > (MB_LEADER_LOW - MB_MARGIN ))) {
    return "M"; // Mitsubishi
  }
  return "";
}

int16_t M5StackIRdecode::decodeBit(int16_t d0, int16_t d1, boolean dm) {
  int16_t tl, ts;

  if ((d0 < TIC_MIN) || (d1 < TIC_MIN) || (d0 > TIC_MAX)) {
    return -1;
  }

  ts = (d0 > d1) ? d1 : d0;
  tl = (d0 > d1) ? d0 : d1;
  if (dm) { // mitsubishi
    return (tl - ts) > (ts * 4) ? 1 : 0;
  } else { //  nec kaden sony
    return (tl - ts) > (ts / 2) ? 1 : 0;
  }
}

boolean M5StackIRdecode::parseData(rmt_item32_t *item, size_t rxSize) {
  int16_t index = 0;
  int16_t dbit, i;
  uint8_t dbyte;
  boolean dmode;

  rmttmp = checkLeader(item[0]);
  if (rmttmp.equals("")) {
    return false;
  }
  if (rmttmp.equals("M")) {
    dmode = true;
    index = 0;
  } else {
    dmode = false;
    index = 1;
  }
  for (i = 0, dbyte = 0; item[index].duration1 > 0; index++) {
    dbit = decodeBit(item[index].duration0, item[index].duration1, dmode);
    if (dbit < 0) {
      return false;
    }
    dbyte = dbyte << 1 | dbit;
    i++;
    if (i > 7) {
      if (dbyte < 0x10) {
        rmttmp.concat('0');
      }
      rmttmp.concat(String(dbyte, HEX));
      dbyte = 0;
      i = 0;
    }
  }
  if (index == 1) { // no data
    return false;
  }
  if (i > 2) { // for SONY format
    rmttmp.concat(String(dbyte, HEX));
  }
  _ircode = rmttmp;
  return true;
}
