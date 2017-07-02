#ifndef __VOIPA_CDR_H__
#define __VOIPA_CDR_H__



typedef struct voipa_mass_ {
    char aisdn[24];
    char ahome[24];
    char aimsi[16];
    char aimei[16];
    char aneid[24];
    char amcc[16];
    char amnc[16];
    char alacs[16];
    char acells[16];
    char alace[16];
    char acelle[16];
    char apc[16];
    char bisdn[24];
    char bisdnx[24];
    char bhome[24];
    char btisdn[48];
    char bimsi[16];
    char bneid[24];
    char bpc[16];
    char acttype[16];
    char starttime[24];
    char endtime[24];
    char duration[16];
    char vocfile[256];
    char extinfo[48];
    char result[16];
    char cause[16];
    char netid[16];
    char smslan[16];
    char smstext[2048];
} voipa_mass_t;


typedef struct voipa_case_ {
    char mntid[16];
    char aisdn[24];
    char ahome[24];
    char aimsi[16];
    char aimei[16];
    char aneid[24];
    char amcc[16];
    char amnc[16];
    char alacs[16];
    char acells[16];
    char alace[16];
    char acelle[16];
    char apc[16];
    char bisdn[24];
    char bisdnx[24];
    char bhome[24];
    char btisdn[48];
    char bimsi[16];
    char bneid[24];
    char bpc[16];
    char acttype[16];
    char starttime[24];
    char endtime[24];
    char duration[16];
    char vocfile[256];
    char extinfo[48];
    char result[16];
    char cause[16];
    char netid[16];
    char smslan[16];
    char smstext[512];
} voipa_case_t;


typedef union
{
    voipa_case_t voip_case;
    voipa_mass_t voip_mass;
}st_voipa_cdr_t;

void *thread_voipa_cdr(void *arg);




#endif
