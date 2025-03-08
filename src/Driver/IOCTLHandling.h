#pragma once


extern void LoadIDT(PIDT pIdt);

extern void IntEight();


NTSTATUS IhIOCTLHandler(PDEVICE_OBJECT deviceObject, PIRP pIrp);