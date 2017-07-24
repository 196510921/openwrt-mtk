#include "struct_test.h"


int main(int argc, char* argv[])
{
    uint1_data test_data;
	uint8_t buf[8] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08};

	test_data.param_id = 0x0001;
	test_data.v_len    = 0x08;
	memcpy(test_data.val,buf, 8);

	
}





