#ifndef ERODE_H
#define ERODE_H

#include "filters.h"

class Erode : public Filter{
public:
	typedef Filter super;
	virtual const char* getName();
	int source;
	virtual int get_source();
	virtual void set_source_id(const char *source);
};

#endif