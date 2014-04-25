/*
 * boardsupport.h
 *
 * Created: 20/03/2013 12:14:04
 *  Author: sfx
 * 
 * A place for all central data structures, system-specific initialisation and access methods. 
 * To do: Maybe should be renamed - system_support.h ?
 */ 


#ifndef BOARDSUPPORT_H_
#define BOARDSUPPORT_H_

#include "central_data.h"

void initialise_board(central_data_t* centralData);

#endif /* BOARDSUPPORT_H_ */