#include "mfi_module_info.h"
#include <stdlib.h>

Module_t Module={NULL,0,0};

void initModuleInfo(){
	Module.number = 3;
	int i=0;
	Module.Module_Info_p = (_Module_Info_p)malloc(sizeof( _Module_Info_p ) * 3);
	for(i=0; i<3; i++){
		Module.Module_Info_p[i].mod_ip = i;
	}
}