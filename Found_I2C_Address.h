/*
 * Found_I2C_Address.h
 *
 *  Created on: Dec 20, 2024
 *      Author: Administrator
 */

#ifndef INC_FOUND_I2C_ADDRESS_H_
#define INC_FOUND_I2C_ADDRESS_H_
//***********************************************************
typedef struct {
	int address, state;
}SEARCHFORADDRESS;

SEARCHFORADDRESS searchforaddress = {0};
//***********************************************************
int FoundAddres(I2C_HandleTypeDef *hi2c) {
	for(searchforaddress.address = 0; searchforaddress.address < 255; searchforaddress.address++) {
		searchforaddress.state = HAL_I2C_IsDeviceReady(hi2c, searchforaddress.address, 10, HAL_MAX_DELAY);
		if(searchforaddress.state == HAL_OK) {break;}
	}
	if(searchforaddress.address == 255) {return 0;}
	else {return searchforaddress.address;}
}
//***********************************************************
#endif /* INC_FOUND_I2C_ADDRESS_H_ */
