
#ifndef _ASN_H_
#define _ASN_H_

typedef void (*ASN_CALLBACK)(unsigned int tag, int constructed, unsigned char * content, int contentlen, unsigned int * layer, int curlayer, void * para);

int deasn(unsigned char * pkt, int len, void * para, unsigned int * layer, int cur_layer, int max_layers, ASN_CALLBACK asn_callback);

#endif
