#ifndef IRDCTYPE_H
#define IRDCTYPE_H

#define NUMOFBUTTONS 32
#define NUMOFARGS 64

#define VENDORID 25171
#define PRODUCTID 256

typedef struct config_item_str {
  int sel;
  char* executable;
  char** args;
} config_item;

enum { IRDC_OK,
    IRDC_SCANERR,
    IRDC_DEVERR,
    IRDC_READERR,
    IRDC_NODEVERR,
    IRDC_OPTERR,
    IRDC_MEMERR,
    IRDC_NOCNFERR,
    IRDC_CNFREADERR,
    IRDC_CNFLONGERR,
    IRDC_TOOARGSERR,
    IRDC_DAEMERR
     };

#endif
