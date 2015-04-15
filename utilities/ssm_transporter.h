#ifndef __SSM_TRANSPORTER_H__
#define __SSM_TRANSPORTER_H__

#include "ssm2hat.h"

typedef struct _ssmt_header {
	int head;
	int id;
	int type;
	size_t size;
	double time;
} SSMT_header;

typedef struct _ssmt_newsensor {
	char name[30];
	int suid;
	size_t size;
	size_t property_size;
	double time;
	double cycle;
} SSMT_newsensor;

typedef struct _reg_sensor *RegSensPtr;
typedef struct _reg_sensor {
	int id;
	char name[30];
	int suid;
	SSM_sid sid;
	size_t size;
	size_t property_size;
	int tid;
	int wp;
	int rp;
	double time;
	char *data;
	char *property;
} RegisterdSensor;

#define SSMT_HEAD 0x55
#define SSMT_NEW 0
#define SSMT_DATA 1
#define SSMT_PROPERTY 2

#endif
