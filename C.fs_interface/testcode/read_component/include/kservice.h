#ifndef _K_SERVICE_
#define _K_SERVICE_

void kservice_uart_write(char *fmt, ...);
void kservice_reg_compt(char* compt_name);
void kservice_unreg_compt(char* compt_name);

#endif
