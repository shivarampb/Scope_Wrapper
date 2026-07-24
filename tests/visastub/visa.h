// visa.h — minimal VISA test double
//
// A stand-in for the IVI VISA header sufficient to compile ScopeCore + plugins
// and run them against an in-process SCPI simulator (visastub.cpp). It is used
// ONLY for host-free testing; production builds use the real vendor VISA.

#ifndef VISASTUB_VISA_H
#define VISASTUB_VISA_H

typedef unsigned int  ViSession;
typedef ViSession*    ViPSession;
typedef int           ViStatus;
typedef unsigned int  ViUInt32;
typedef unsigned int  ViAccessMode;
typedef unsigned int  ViAttr;
typedef unsigned long ViAttrState;
typedef char*         ViRsrc;
typedef unsigned char* ViBuf;

#define VI_NULL             0
#define VI_SUCCESS          0
#define VI_SUCCESS_MAX_CNT  0x3FFF0006L

// Error codes (negative so `status < VI_SUCCESS` holds, as in real VISA).
#define VI_ERROR_RSRC_NFOUND   (-1073807343)
#define VI_ERROR_TMO           (-1073807339)
#define VI_ERROR_CONN_LOST     (-1073807194)
#define VI_ERROR_INV_RSRC_NAME (-1073807342)

// Attributes / serial constants (unused by the simulator; any value works).
#define VI_ATTR_TMO_VALUE      0x3FFF001A
#define VI_ATTR_ASRL_BAUD      0x3FFF0021
#define VI_ATTR_ASRL_DATA_BITS 0x3FFF0022
#define VI_ATTR_ASRL_PARITY    0x3FFF0023
#define VI_ATTR_ASRL_STOP_BITS 0x3FFF0024
#define VI_ASRL_PAR_NONE       0
#define VI_ASRL_STOP_ONE       10

#ifdef __cplusplus
extern "C" {
#endif

ViStatus viOpenDefaultRM(ViPSession vi);
ViStatus viOpen(ViSession sesn, ViRsrc name, ViAccessMode mode, ViUInt32 timeout, ViPSession vi);
ViStatus viClose(ViSession vi);
ViStatus viSetAttribute(ViSession vi, ViAttr attrName, ViAttrState attrValue);
ViStatus viWrite(ViSession vi, ViBuf buf, ViUInt32 count, ViUInt32* retCount);
ViStatus viRead(ViSession vi, ViBuf buf, ViUInt32 count, ViUInt32* retCount);

#ifdef __cplusplus
}
#endif

#endif // VISASTUB_VISA_H
