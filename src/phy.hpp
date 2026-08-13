#pragma once

#include <Arduino_10BASE_T1S.h>

extern TC6::TC6_Io t1s_io;
extern TC6::TC6_Arduino_10BASE_T1S t1s_phy;

void OnPlcaStatus(bool success, bool plcaStatus);
void initPhy();
