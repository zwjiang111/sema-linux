#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>

// Dummy definition (you must link to the actual EAPI library)
#define EAPI_ID_BOARD_NAME_STR 0x00000001
#define EAPI_ID_HWMON_CPU_TEMP 0x20000

#define EAPI_STATUS_SUCCESS     0

uint32_t EApiLibInitialize(void);

uint32_t EApiLibUnInitialize(void);

uint32_t EApiBoardGetStringA(uint32_t Id, char* pBuffer, uint32_t* pBufLen);

uint32_t EApiBoardGetValue(uint32_t Id, uint32_t *pValue);

uint32_t EApiBoardGetVoltageMonitor(uint32_t id, uint32_t *pVoltage, char *pBuf, uint32_t size);

uint32_t EApiStorageAreaRead(uint32_t Id, uint32_t Offset, void *pBuffer, uint32_t BufLen, uint32_t  ByteCnt);

uint32_t EApiStorageAreaWrite(uint32_t Id, uint32_t Offset, void *pBuffer, uint32_t ByteCnt);

uint32_t EApiGPIOGetDirection(uint32_t Id, uint32_t Bitmask, uint32_t *pDirection);

uint32_t EApiGPIOSetDirection(uint32_t Id, uint32_t Bitmask, uint32_t Direction);

uint32_t EApiSmartFanGetPWMSetpoints(int id, int *pwm_Level1, int *pwm_Level2, int *pwm_Level3, int *pwm_Level4);

uint32_t EApiSmartFanSetPWMSetpoints(int id, int pwm_Level1, int pwm_Level2, int pwm_Level3, int pwm_Level4);

uint32_t EApiWDogStart(uint32_t delay, uint32_t EventTimeout, uint32_t ResetTimeout);

uint32_t EApiWDogStop(void);

uint32_t EApiVgaGetBacklightBrightness(uint32_t Id, uint32_t *pBright);

uint32_t EApiVgaSetBacklightBrightness(uint32_t Id, uint32_t Bright);

uint32_t EApiI2CWriteReadRaw(uint32_t Id, uint8_t Addr, void *pWBuffer, uint32_t WriteBCnt, void *pRBuffer, uint32_t RBufLen, uint32_t  ReadBCnt);

#define NUM_THREADS 16
#define BUF_SIZE 64

volatile sig_atomic_t stop_requested = 0;

void handle_sigint(int sig) {
    printf("\nSIGINT request received. Shutting down...\n");
    stop_requested = 1;
}

void* thread_func(void* arg) {
    int thread_id = *(int*)arg;
    free(arg);

    while (!stop_requested) {
	uint32_t ret;
	
	char buffer[BUF_SIZE] = {0};
        uint32_t size = BUF_SIZE;
        ret = EApiBoardGetStringA(EAPI_ID_BOARD_NAME_STR, buffer, &size);

        if (ret == EAPI_STATUS_SUCCESS) {
            printf("Thread %d: Board name = %s\n", thread_id, buffer);
	    if (strncmp(buffer, "COMHPC-sIDH", 11) != 0) {
            	fprintf(stderr, "Thread %d: Board name does not match 'COMHPC-sIDH'. Got '%s' instead. Exiting.\n",
                        thread_id, buffer);
            	stop_requested = 1;
            	return NULL;
            }
	} 
	else {
            fprintf(stderr, "Thread %d: Failed to get board name. Error = %u — exiting program.\n", thread_id, ret);
	    EApiLibUnInitialize();
            exit(EXIT_FAILURE);
        }
	
	// Reading the CPU temperature
	uint32_t Value = 5555;

	ret = EApiBoardGetValue(EAPI_ID_HWMON_CPU_TEMP, &Value);

	if (ret == EAPI_STATUS_SUCCESS) {
            printf("Thread %d: CPU Temperature = %d K\n", thread_id, Value);

	    if (Value < 2732 || Value > 3331) {
                fprintf(stderr, "Thread %d: CPU Temp is below/above the limit. Got '%d'\n",
                        thread_id, Value);
                stop_requested = 1;
		return NULL;
            }
        } else {
            fprintf(stderr, "Thread %d: Failed to get CPU Temperature. Error = %u — exiting program.\n", thread_id, ret);
	    EApiLibUnInitialize();
            exit(EXIT_FAILURE);
        }

	// Set and Get GPIO dir
	uint32_t dir;
	ret = EApiGPIOSetDirection(0, 1 << 4, 1);
	if (ret == EAPI_STATUS_SUCCESS) {
            printf("Thread %d: GPIO Pin5 set to Input\n", thread_id);
		
            ret = EApiGPIOGetDirection(0, 1 << 4, &dir);

	    if (ret == EAPI_STATUS_SUCCESS) 
	    {
	    	if (dir & (1 << 4))
		{
			printf("Thread %d: Input direction updated successfully\n", thread_id);
            	}
		else
		{
			fprintf(stderr, "Thread %d: Failed in GPIO! Direction we got'%d'\n",thread_id, dir);
               		stop_requested = 1;
			return NULL;
		}
            } 
	    else 
	    {
            	fprintf(stderr, "Thread %d: Failed to get GPIO direction. Error = %u — exiting program.\n", thread_id, ret);
	    	EApiLibUnInitialize();
            	exit(EXIT_FAILURE);
            }
        } else {
            fprintf(stderr, "Thread %d: Failed to set GPIO direction. Error = %u — exiting program.\n", thread_id, ret);
	    EApiLibUnInitialize();
            exit(EXIT_FAILURE);
        }

	//////////////////////////////////////////////////////////////////
	// Reading voltage
        char Vmbuf[32] = {0};
        uint32_t Voltage;

        ret = EApiBoardGetVoltageMonitor(0, &Voltage, Vmbuf, 32);

        if (ret == EAPI_STATUS_SUCCESS) {
            printf("Thread %d: Voltage: %u mv\nDescription: %s\n", thread_id, Voltage, Vmbuf);

            if (Voltage <= 0 || Voltage > 10000) {
                fprintf(stderr, "Thread %d: Board Voltage is below/above the limit. Got '%d'\n",
                        thread_id, Voltage);
                stop_requested = 1;
                return NULL;
            }
        } else {
            fprintf(stderr, "Thread %d: Failed to get board voltage. Error = %u — exiting program.\n", thread_id, ret);
            EApiLibUnInitialize();
            exit(EXIT_FAILURE);
        }

        /////////////////////
	// Storage write and read

	char memcap[4096] = {0};
	char buflen = 0;
	char *Buffer = malloc(100);  // Allocate 100 bytes on heap

	if (Buffer != NULL) {
    		strcpy(Buffer, "adlink");
	}	
	buflen = strlen(Buffer);
	ret = EApiStorageAreaWrite(1,0,Buffer,buflen);
    	free(Buffer);
	if (ret == EAPI_STATUS_SUCCESS) {
		printf("Thread %d: Data Written Successfully\n",thread_id);

		ret = EApiStorageAreaRead(1, 0, memcap, 4096, 6);

		if (ret == EAPI_STATUS_SUCCESS) 
	    	{
			printf("Thread %d: User region string = %s\n", thread_id, memcap);
			if (strncmp(memcap, "adlink", 6) != 0)
			{
				fprintf(stderr, "Thread %d: Storage string does not match 'adlink'. Got '%s' instead. Exiting.\n",
                     		   thread_id, memcap);
            			stop_requested = 1;
            			return NULL;
			}
		}
		else 
		{
                	fprintf(stderr, "Thread %d: Failed to get storage string. Error = %u — exiting program.\n", thread_id, ret);
	        	EApiLibUnInitialize();
            		exit(EXIT_FAILURE);
        	}
	}
	else 
	{
        	fprintf(stderr, "Thread %d: Failed to write storage string. Error = %u — exiting program.\n", thread_id, ret);
		EApiLibUnInitialize();
        	exit(EXIT_FAILURE);
        }
	////////////////////////////////////////////////////////////////////////
	
	/////////Fan get and set/////////
 	int fid = 0, Level1, Level2, Level3, Level4;

	ret = EApiSmartFanSetPWMSetpoints(0, 20, 40, 60, 80);	
        if (ret == EAPI_STATUS_SUCCESS) {
            printf("Thread %d: PWM Levels set successfully\n", thread_id);

	    ret = EApiSmartFanGetPWMSetpoints(fid, &Level1, &Level2, &Level3, &Level4);
	    
	    if(ret == EAPI_STATUS_SUCCESS)
	    {
		    printf("Thread %d: Fan ID: %d (CPU fan)\nLevel1: %d\nLevel2: %d\nLevel3: %d\nLevel4: %d\n", thread_id, fid, Level1, Level2, Level3, Level4);

		    if(Level1 != 20 || Level2 != 40 || Level3 != 60 || Level4 != 80)
		    {
			fprintf(stderr, "Thread %d: Fan Level does not matches. Got '%d %d %d %d' instead. Exiting.\n", thread_id, Level1, Level2, Level3, Level4);
                        stop_requested = 1;
                        return NULL;
		    }
	    }
        } else {
            fprintf(stderr, "Thread %d: Failed to set PWM levels in CPU fan. Error = %u — exiting program.\n", thread_id, ret);
            EApiLibUnInitialize();
            exit(EXIT_FAILURE);
        }

	///////////////////////////////////////////////////////////////////////
#if 0	
	///////////// I2C Operation////////////////////////////////////////////
	void* pBuffer, *RpBuffer;
	pBuffer = calloc(3, sizeof(unsigned char));

	((unsigned char*)(pBuffer))[0] = 0x00;
	((unsigned char*)(pBuffer))[1] = 0x01;
	((unsigned char*)(pBuffer))[2] = 0x02;

	ret = EApiI2CWriteReadRaw(1, 0x50 << 1, pBuffer, 3, NULL, 0, 0);
	if(ret == EAPI_STATUS_SUCCESS)
	{
		printf("\nThread id %d:The I2C Write Transfer command is completed\n",thread_id);

		RpBuffer = calloc(2, sizeof(unsigned char));
		((unsigned char*)(pBuffer))[0] = 0x00;
		ret = EApiI2CWriteReadRaw(1, 0x50 << 1, pBuffer, 1, RpBuffer, 2, 2);
		if(ret == EAPI_STATUS_SUCCESS)
		{
			printf("Read data:\n\n");
			for (int o = 0; o < 2; o++)
			{
				printf("Thread id %d-> %02d : %02x\n", thread_id, o, ((unsigned char*)(RpBuffer))[o]);
			}

			if(((unsigned char*)(RpBuffer))[0] != 1 || ((unsigned char*)(RpBuffer))[1] != 2)
			{
				 fprintf(stderr, "Thread %d: I2c data does not matches. Got '%u %u' instead. Exiting.\n", thread_id, ((unsigned char*)(RpBuffer))[0], ((unsigned char*)(RpBuffer))[1]);
				 stop_requested = 1;
				 free(RpBuffer);
                        	 return NULL;
			}
			free(pBuffer);
			free(RpBuffer);
		}
		else
		{
			free(RpBuffer);
			fprintf(stderr, "Thread %d: Failed to get the EEPROM data using I2C. Error = %u — exiting program.\n", thread_id, ret);
            		EApiLibUnInitialize();
            		exit(EXIT_FAILURE);
		}
	}
	else
	{
		 fprintf(stderr, "Thread %d: Failed to write the EEPROM data using I2C. Error = %u — exiting program.\n", thread_id, ret);
                 EApiLibUnInitialize();
                 exit(EXIT_FAILURE);
	}

	//////////////////////////////////////////////////////////////////////

	///////////////////// Wdt get/set/////////////////////////////////////
	unsigned int Timeout = 10;

	ret = EApiWDogStart(0, 0, Timeout);

	if(ret == EAPI_STATUS_SUCCESS)
	{
		printf("Thread %d:Run-time Watchdog Started with : %u seconds \n",thread_id, Timeout);
		ret = EApiWDogStop();
		if(ret == EAPI_STATUS_SUCCESS)
		{
			printf("Thread %d: Watchdog Stopped Successfully\n",thread_id);
		}
		else
		{
			fprintf(stderr, "Thread %d: Failed to stop the watchdog. Error = %u — exiting program.\n", thread_id, ret);
                        EApiLibUnInitialize();
                        exit(EXIT_FAILURE);
		}
	}
	else
	{
		fprintf(stderr, "Thread %d: Failed to set the watchdog timer. Error = %u — exiting program.\n", thread_id, ret);
                EApiLibUnInitialize();
                exit(EXIT_FAILURE);
	}
	//////////////////////////////////////////////////////////////////////
#endif
	
	/////////////// backlight get/set value///////////////////////////////

	unsigned int brightness;

        ret = EApiVgaSetBacklightBrightness(0, 222);
        if (ret == EAPI_STATUS_SUCCESS) {
            printf("Thread %d: Current Backlight Brightness is set to %u\n", thread_id, 222);
		
            ret = EApiVgaGetBacklightBrightness(0, &brightness);

            if(ret == EAPI_STATUS_SUCCESS)
            {
                    printf("Thread %d: Current Backlight Brightness is %u\n", thread_id, brightness);

                    if(brightness != 222)
                    {
                        fprintf(stderr, "Thread %d: Backlight brightness value is not matched. Got '%u' instead. Exiting.\n", thread_id, brightness);
                        stop_requested = 1;
                        return NULL;
                    }
            }
	    else
	    {
		    fprintf(stderr, "Thread %d: Failed to get the backlight brightness value. Error = %u — exiting program.\n", thread_id, ret);
            	    EApiLibUnInitialize();
            	    exit(EXIT_FAILURE);
	    }
        } else {
            fprintf(stderr, "Thread %d: Failed to set the backlight brightness value. Error = %u — exiting program.\n", thread_id, ret);
            EApiLibUnInitialize();
            exit(EXIT_FAILURE);
        }

	//////////////////////////////////////////////////////////////////////	
        
	// Optional delay
        usleep(10000);  // 10 ms
    }

    return NULL;
}

int main() {
	
    int ret;

    signal(SIGINT, handle_sigint);  // Register SIGINT handler

    if ((ret = EApiLibInitialize()) != 0)
    {
 	printf("Initialization Failed. Error: 0x%X\n", ret);
	exit(0);
    }
    
    pthread_t threads[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        int* id = malloc(sizeof(int));
        if (!id) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        *id = i;

        if (pthread_create(&threads[i], NULL, thread_func, id) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    // Wait forever (or until manually killed)
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    EApiLibUnInitialize();
    return 0;
}
