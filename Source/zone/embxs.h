#ifndef EMBXS_H 
#define EMBXS_H 
const char *getItemName(unsigned itemid); 
XS(XS_qc_getItemName); /* prototype to pass -Wmissing-prototypes */ 
EXTERN_C XS(boot_qc); /* prototype to pass -Wmissing-prototypes */ 
#endif // EMBXS_H