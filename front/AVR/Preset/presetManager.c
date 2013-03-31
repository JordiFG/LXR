/*
 * presetManager.c
 *
 * Created: 09.05.2012 16:06:19
 *  Author: Julian
 */ 
#include "../config.h"
#include "PresetManager.h"
#include "../Hardware\SD/ff.h"
#include <stdio.h>
#include "..\Menu\menu.h"
#include <util\delay.h>
#include "..\Hardware\lcd.h"
#include <avr/pgmspace.h>
#include "../frontPanelParser.h"
#include <stdlib.h>
#include <util/atomic.h> 
#include "../IO/uart.h"
#include "../IO/din.h"
#include "../IO/dout.h"
#include "../midiLfo.h"
#include "../Hardware\timebase.h"

FATFS preset_Fatfs;		/* File system object for the logical drive */
FIL preset_File;		/* place to hold 1 file*/

char preset_currentName[8];





#define NUM_TRACKS 7
#define NUM_PATTERN 8
#define STEPS_PER_PATTERN 128

//----------------------------------------------------
void preset_init()
{

	//mount the filesystem	
	f_mount(0,(FATFS*)&preset_Fatfs);
	
	
}

//----------------------------------------------------
void preset_saveDrumset(uint8_t presetNr)
{
#if USE_SD_CARD	
	//filename in 8.3  format
	char filename[13];
	sprintf(filename,"p%03d.snd",presetNr);
	//open the file
	f_open((FIL*)&preset_File,filename,FA_CREATE_ALWAYS | FA_WRITE);


	unsigned int bytesWritten;
	//write preset name to file
	f_write((FIL*)&preset_File,(void*)preset_currentName,8,&bytesWritten);
	//write the preset data
	//caution. the parameter data is in a struct. every 1st value has to be saved (param value)
	//every 2nd ommited (max value)
	int i;
	for(i=0;i<END_OF_SOUND_PARAMETERS;i++)
	{
		f_write((FIL*)&preset_File,&parameters[i].value,1,&bytesWritten);	
	}
	/*
	//save global parameters [samplerate]
	f_write((FIL*)&preset_File,&parameters[PAR_VOICE_DECIMATION1].value,1,&bytesWritten);	
	f_write((FIL*)&preset_File,&parameters[PAR_VOICE_DECIMATION2].value,1,&bytesWritten);	
	f_write((FIL*)&preset_File,&parameters[PAR_VOICE_DECIMATION3].value,1,&bytesWritten);	
	f_write((FIL*)&preset_File,&parameters[PAR_VOICE_DECIMATION4].value,1,&bytesWritten);	
	f_write((FIL*)&preset_File,&parameters[PAR_VOICE_DECIMATION5].value,1,&bytesWritten);	
	f_write((FIL*)&preset_File,&parameters[PAR_VOICE_DECIMATION6].value,1,&bytesWritten);	
	
	*/
	
	//close the file
	f_close((FIL*)&preset_File);
#else
#endif
};

//----------------------------------------------------
void preset_saveGlobals()
{
#if USE_SD_CARD	
	//open the file
	f_open((FIL*)&preset_File,"glo.cfg",FA_CREATE_ALWAYS | FA_WRITE);


	unsigned int bytesWritten;
	int i;
	//f_lseek((FIL*)&preset_File,PAR_BEGINNING_OF_GLOBALS);
	
		
	for(i=PAR_BEGINNING_OF_GLOBALS;(i<NUM_PARAMS);i++)
	{
		f_write((FIL*)&preset_File,&parameters[i].value,1,&bytesWritten);	
	}
	//close the file
	f_close((FIL*)&preset_File);
#else
#endif	
}
//----------------------------------------------------
void preset_loadGlobals()
{
#if USE_SD_CARD		
	//open the file
	FRESULT res = f_open((FIL*)&preset_File,"glo.cfg",FA_OPEN_EXISTING | FA_READ);
	
	if(res!=FR_OK) {
		//file open error... maybe the file does not exist?
		return;		
	} else {
		unsigned int bytesRead;
		int i;
		
	//	f_lseek((FIL*)&preset_File,PAR_BEGINNING_OF_GLOBALS);
		
		for(i=PAR_BEGINNING_OF_GLOBALS;(i<NUM_PARAMS) &&( bytesRead!=0);i++)
		{
			f_read((FIL*)&preset_File,&parameters[i].value,1,&bytesRead);		
		}	
			
		menu_sendAllGlobals();		
		f_close((FIL*)&preset_File);		
	}
	menu_repaintAll();
	return;
#else
	
#endif	
}
//----------------------------------------------------
uint8_t preset_loadDrumset(uint8_t presetNr, uint8_t isMorph)
{
	
#if USE_SD_CARD		
	//filename in 8.3  format
	char filename[13];
	sprintf(filename,"p%03d.snd",presetNr);
	//open the file
	FRESULT res = f_open((FIL*)&preset_File,filename,FA_OPEN_EXISTING | FA_READ);
	
	if(res!=FR_OK)
	{
		//file open error... maybe the file does not exist?
		return 0;
		/*
		lcd_clear();
		lcd_home();
		lcd_string_F(PSTR("Could not"));
		lcd_setcursor(0,2);
		lcd_string_F(PSTR("open preset"));
		_delay_ms(2000);
		*/
		
	}
	else
	{
		
	//	lcd_clear();
	//	lcd_home();
	//	lcd_string_F(PSTR("Loading preset"));
	//	_delay_ms(2000);
		//now read the file content
		unsigned int bytesRead;
		//first the preset name
	//	lcd_clear();
	//	lcd_string_F(PSTR("read name"));
		f_read((FIL*)&preset_File,(void*)preset_currentName,8,&bytesRead);
		//then the data
		int i;
	//	lcd_clear();
	//	lcd_string_F(PSTR("read data"));
		bytesRead = 1;
		
		if(isMorph)
		{
			for(i=0;(i<END_OF_SOUND_PARAMETERS) &&( bytesRead!=0);i++)
			{
				f_read((FIL*)&preset_File,&parameters2[i].value,1,&bytesRead);	
			}	
			/*
			//skip unused parameters -> forward to decimation file  pos
			uint8_t  dummy;
			for(;i<PAR_VOICE_DECIMATION1;i++)
			{
				f_read((FIL*)&preset_File,&dummy,1,&bytesRead);
			}
			
			//load global parameters [samplerate]
			f_read((FIL*)&preset_File,&parameters2[PAR_VOICE_DECIMATION1].value,1,&bytesRead);	
			f_read((FIL*)&preset_File,&parameters2[PAR_VOICE_DECIMATION2].value,1,&bytesRead);	
			f_read((FIL*)&preset_File,&parameters2[PAR_VOICE_DECIMATION3].value,1,&bytesRead);	
			f_read((FIL*)&preset_File,&parameters2[PAR_VOICE_DECIMATION4].value,1,&bytesRead);	
			f_read((FIL*)&preset_File,&parameters2[PAR_VOICE_DECIMATION5].value,1,&bytesRead);	
			f_read((FIL*)&preset_File,&parameters2[PAR_VOICE_DECIMATION6].value,1,&bytesRead);	
*/
		}
		else
		{
			//for(i=0;(i<NUM_PARAMS )&&( bytesRead!=0);i++) //dont read dummy or global params
			for(i=0;(i<END_OF_SOUND_PARAMETERS) &&( bytesRead!=0);i++)
			{
				f_read((FIL*)&preset_File,&parameters[i].value,1,&bytesRead);	
			}	
			/*
			//skip unused parameters -> forward to decimation file  pos
			uint8_t  dummy;
			for(;i<PAR_VOICE_DECIMATION1;i++)
			{
				f_read((FIL*)&preset_File,&dummy,1,&bytesRead);
			}
			
			//load global parameters [samplerate]
			f_read((FIL*)&preset_File,&parameters[PAR_VOICE_DECIMATION1].value,1,&bytesRead);	
			f_read((FIL*)&preset_File,&parameters[PAR_VOICE_DECIMATION2].value,1,&bytesRead);	
			f_read((FIL*)&preset_File,&parameters[PAR_VOICE_DECIMATION3].value,1,&bytesRead);	
			f_read((FIL*)&preset_File,&parameters[PAR_VOICE_DECIMATION4].value,1,&bytesRead);	
			f_read((FIL*)&preset_File,&parameters[PAR_VOICE_DECIMATION5].value,1,&bytesRead);	
			f_read((FIL*)&preset_File,&parameters[PAR_VOICE_DECIMATION6].value,1,&bytesRead);	
			*/
			
			
			
			//special case mod targets
			for(i=0;i<6;i++)
			{
				//velo
				uint8_t value = getModTargetValue(parameters[PAR_VEL_DEST_1+i].value, i);
				uint8_t upper,lower;
				upper = ((value&0x80)>>7) | (((i)&0x3f)<<1);
				lower = value&0x7f;
				frontPanel_sendData(CC_VELO_TARGET,upper,lower);
				
				//LFO
				value = getModTargetValue(parameters[PAR_TARGET_LFO1+i].value,  parameters[PAR_VOICE_LFO1+i].value-1);
				upper = ((value&0x80)>>7) | (((i)&0x3f)<<1);
				lower = value&0x7f;
				frontPanel_sendData(CC_LFO_TARGET,upper,lower);
				
				
			}	
						
		}
		
		//close the file handle
		f_close((FIL*)&preset_File);
	
		//now we need to send the new param values to the cortex and repaint the menu
		//menu_sendAllParameters();
		preset_morph(parameters[PAR_MORPH].value);
	return 1;	
	
		//TODO send program change message
		
	}
	//force complete repaint
	menu_repaintAll();
	return 1;
	
#else
	frontPanel_sendData(PRESET,PRESET_LOAD,presetNr);
#endif
};
//----------------------------------------------------
char* preset_getPatternName(uint8_t presetNr)
{
	
#if USE_SD_CARD	
//filename in 8.3  format
	char filename[13];
	sprintf(filename,"p%03d.pat",presetNr);
	memcpy_P((void*)preset_currentName,PSTR("Empty   "),8);
	//try to open the file
	FRESULT res = f_open((FIL*)&preset_File,filename,FA_OPEN_EXISTING | FA_READ);
	
	if(res!=FR_OK)
	{
		//error opening the file
		
		return NULL;	
	}
	
	//file opened correctly -> extract name (first 8 bytes)
	unsigned int bytesRead;
	f_read((FIL*)&preset_File,(void*)preset_currentName,8,&bytesRead);
	
	
	//close the file handle
	f_close((FIL*)&preset_File);
	
	return (char*)preset_currentName;
	
#else
return "ToDo";
#endif
};
//----------------------------------------------------
char* preset_loadName(uint8_t presetNr, uint8_t what)
{
	if(what == 1) {
		return preset_getPatternName(presetNr);
	} else {
		return preset_getDrumsetName(presetNr);
	}
}
//----------------------------------------------------
char* preset_getDrumsetName(uint8_t presetNr)
{
	
#if USE_SD_CARD		
	//filename in 8.3  format
	char filename[13];
	sprintf(filename,"p%03d.snd",presetNr);
	
	//try to open the file
	FRESULT res = f_open((FIL*)&preset_File,filename,FA_OPEN_EXISTING | FA_READ);
	
	if(res!=FR_OK)
	{
		//error opening the file
		memcpy_P((void*)preset_currentName,PSTR("Empty   "),8);
		return NULL;	
	}
	
	//file opened correctly -> extract name (first 8 bytes)
	unsigned int bytesRead;
	f_read((FIL*)&preset_File,(void*)preset_currentName,8,&bytesRead);
	
	
	//close the file handle
	f_close((FIL*)&preset_File);
	
	return (char*)preset_currentName;
#else

return "ToDo";
#endif
};
//----------------------------------------------------
/** request step data from the cortex via uart and save it in the provided step struct*/
void preset_queryStepDataFromSeq(uint16_t stepNr)
{
//	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) 
	{
		frontParser_newSeqDataAvailable = 0;
	}	
	//request step data
	//the max number for 14 bit data is 16383!!!
	//current max step nr is 128*7*8 = 7168
//	_delay_ms(10);
	//frontPanel_sendByte(SYSEX_REQUEST_STEP_DATA);
	frontPanel_sendByte((stepNr>>7)&0x7f);		//upper nibble 7 bit
	frontPanel_sendByte(stepNr&0x7f);	//lower nibble 7 bit
//asm volatile ("nop");
//_delay_ms(100);
	//wait until data arrives
	uint8_t newSeqDataLocal = 0;
	uint16_t now = time_sysTick;
	while((newSeqDataLocal==0))
	{
		//we need an atomic access to the frontParser_newSeqDataAvailable value
		//since it is shared with the interrupt
		//otherwise this transmission won't work and the AVR will stall
	//	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) 
	//we have to call the uart parser to handle incoming messages from the sequencer
		uart_checkAndParse();
		
		newSeqDataLocal = frontParser_newSeqDataAvailable;
		
		if(time_sysTick-now >= 31)
		{
			//timeout
			now = time_sysTick;
			//request step again
		//	frontPanel_sendByte(SYSEX_REQUEST_STEP_DATA);
			frontPanel_sendByte((stepNr>>7)&0x7f);		//upper nibble 7 bit
			frontPanel_sendByte(stepNr&0x7f);
		}
		
		
	//	asm volatile ("nop");
		//_delay_ms(500);
	}
	
	
	//the new data is now in frontParser_stepData
	//reset frontParser_newSeqDataAvailable
	//frontParser_newSeqDataAvailable = 0;
	
}
//----------------------------------------------------
void preset_sendMainStepDataToSeq(uint16_t stepNr, uint16_t mainStepData)
{
	frontPanel_sendByte(mainStepData	& 0x7f);
	frontPanel_sendByte((mainStepData>>7)	& 0x7f);
	frontPanel_sendByte((mainStepData>>14)	& 0x7f);
}
//----------------------------------------------------
/** send step data from SD card to the sequencer*/
void preset_sendStepDataToSeq(uint16_t stepNr)
{
	frontPanel_sendByte(frontParser_stepData.volume	& 0x7f);
	frontPanel_sendByte(frontParser_stepData.prob	& 0x7f);
	frontPanel_sendByte(frontParser_stepData.note	& 0x7f);

	frontPanel_sendByte(frontParser_stepData.param1Nr	& 0x7f);
	frontPanel_sendByte(frontParser_stepData.param1Val	& 0x7f);

	frontPanel_sendByte(frontParser_stepData.param2Nr	& 0x7f);
	frontPanel_sendByte(frontParser_stepData.param2Val	& 0x7f);

	//now the MSBs from all 7 values
	frontPanel_sendByte( 	((frontParser_stepData.volume 	& 0x80)>>7) |
							((frontParser_stepData.prob	 	& 0x80)>>6) |
							((frontParser_stepData.note	 	& 0x80)>>5) |
							((frontParser_stepData.param1Nr	& 0x80)>>4) |
							((frontParser_stepData.param1Val	& 0x80)>>3) |
							((frontParser_stepData.param2Nr	& 0x80)>>2) |
							((frontParser_stepData.param2Val	& 0x80)>>1)
							);
}	
//----------------------------------------------------
void preset_queryPatternInfoFromSeq(uint8_t patternNr, uint8_t* next, uint8_t* repeat)
{
	frontParser_newSeqDataAvailable = 0;
	//request pattern info
	frontPanel_sendByte(patternNr);	

	//wait until data arrives
	uint8_t newSeqDataLocal = 0;
	uint16_t now = time_sysTick;
	while((newSeqDataLocal==0))
	{
		//we have to call the uart parser to handle incoming messages from the sequencer
		uart_checkAndParse();
		
		newSeqDataLocal = frontParser_newSeqDataAvailable;
		
		if(time_sysTick-now >= 31)
		{
			//timeout
			now = time_sysTick;
			//request step again
			frontPanel_sendByte(patternNr);	
		}
	}
	
	//the stepdata struct is used as buffer for the data
	*next = frontParser_stepData.volume; 
	*repeat =  frontParser_stepData.prob;
	
	//the new data is now in frontParser_stepData
	//reset frontParser_newSeqDataAvailable
	//frontParser_newSeqDataAvailable = 0;		
	
	
}
//----------------------------------------------------
void preset_queryMainStepDataFromSeq(uint16_t stepNr, uint16_t *mainStepData)
{
	frontParser_newSeqDataAvailable = 0;

	//request step data
	//the max number for 14 bit data is 16383!!!
	//current max step nr is 7*8 = 56
	frontPanel_sendByte((stepNr>>7)&0x7f);		//upper nibble 7 bit
	frontPanel_sendByte(stepNr&0x7f);	//lower nibble 7 bit

	//wait until data arrives
	uint8_t newSeqDataLocal = 0;
	uint16_t now = time_sysTick;
	while((newSeqDataLocal==0))
	{
		//we have to call the uart parser to handle incoming messages from the sequencer
		uart_checkAndParse();
		
		newSeqDataLocal = frontParser_newSeqDataAvailable;
		
		if(time_sysTick-now >= 31)
		{
			//timeout
			now = time_sysTick;
			//request step again
			frontPanel_sendByte((stepNr>>7)&0x7f);		//upper nibble 7 bit
			frontPanel_sendByte(stepNr&0x7f);
		}
	}
	
	*mainStepData = (frontParser_stepData.volume<<8) | frontParser_stepData.prob;
	
	
	//the new data is now in frontParser_stepData
	//reset frontParser_newSeqDataAvailable
	//frontParser_newSeqDataAvailable = 0;
};
//----------------------------------------------------
void preset_savePattern(uint8_t presetNr)
{
	
#if USE_SD_CARD	
	lcd_clear();
	lcd_home();
	lcd_string_F(PSTR("Saving pattern"));
	
	
	//filename in 8.3  format
	char filename[13];
	sprintf(filename,"p%03d.pat",presetNr);
	//open the file
	f_open((FIL*)&preset_File,filename,FA_CREATE_ALWAYS | FA_WRITE);


	uint16_t bytesWritten;
	//write preset name to file
	f_write((FIL*)&preset_File,(void*)preset_currentName,8,&bytesWritten);
#if USE_LFO	
	lfo_disable(1);
#endif	
	//write the preset data
	//initiate the sysex mode
	
	while( (frontParser_midiMsg.status != SYSEX_START))
	{
		frontPanel_sendByte(SYSEX_START);
		uart_checkAndParse();
	}		
	_delay_ms(10);
	//enter step data mode
	frontPanel_sendByte(SYSEX_REQUEST_STEP_DATA);
	frontPanel_sysexMode = SYSEX_REQUEST_STEP_DATA;
	
	uint8_t percent=0;
	char text[5];
	uint16_t i;
	for(i=0;i<(STEPS_PER_PATTERN*NUM_PATTERN*NUM_TRACKS);i++)
	{
		if((i&0x80) == 0)
		{
			//set cursor to beginning of row 2
			lcd_setcursor(0,2);
			//print percent value
			percent = i * (100.f/(STEPS_PER_PATTERN*NUM_PATTERN*NUM_TRACKS));
			itoa(percent,text,10);
			lcd_string(text);
		}			
		
		
		
		//get next data chunk and write it to file
		preset_queryStepDataFromSeq(i);
		f_write((FIL*)&preset_File,(const void*)&frontParser_stepData,sizeof(StepData),&bytesWritten);	
	}
	
	//end sysex mode
	frontPanel_sendByte(SYSEX_END);
	frontParser_midiMsg.status = 0;
	//now the main step data
	while( (frontParser_midiMsg.status != SYSEX_START))
	{
		frontPanel_sendByte(SYSEX_START);
		uart_checkAndParse();
	}	
	_delay_ms(50);	
	frontPanel_sendByte(SYSEX_REQUEST_MAIN_STEP_DATA);
	frontPanel_sysexMode = SYSEX_REQUEST_MAIN_STEP_DATA;
	
	uint16_t mainStepData;
	for(i=0;i<(NUM_PATTERN*NUM_TRACKS);i++)
	{
		//get next data chunk and write it to file
		preset_queryMainStepDataFromSeq(i, &mainStepData);
		f_write((FIL*)&preset_File,(const void*)&mainStepData,sizeof(uint16_t),&bytesWritten);	
	}
		
	//end sysex mode
	frontPanel_sendByte(SYSEX_END);
	frontParser_midiMsg.status = 0;
	
	//----- pattern info (next/repeat) ------
	
	while( (frontParser_midiMsg.status != SYSEX_START))
	{
		frontPanel_sendByte(SYSEX_START);
		uart_checkAndParse();
	}	
	_delay_ms(50);	
	frontPanel_sendByte(SYSEX_REQUEST_PATTERN_DATA);
	frontPanel_sysexMode = SYSEX_REQUEST_PATTERN_DATA;
	
	uint8_t next;
	uint8_t repeat;
	for(i=0;i<(NUM_PATTERN);i++)
	{
		//get next data chunk and write it to file
		preset_queryPatternInfoFromSeq(i, &next, &repeat);
		f_write((FIL*)&preset_File,(const void*)&next,sizeof(uint8_t),&bytesWritten);	
		f_write((FIL*)&preset_File,(const void*)&repeat,sizeof(uint8_t),&bytesWritten);
	}
		
	//end sysex mode
	frontPanel_sendByte(SYSEX_END);
	frontParser_midiMsg.status = 0;
	
	
	//now we need to save the shuffle setting
	f_write((FIL*)&preset_File,(const void*)&parameters[PAR_SHUFFLE].value,sizeof(uint8_t),&bytesWritten);	

#if USE_LFO	
	lfo_disable(0);
#endif
	
	
	//close the file
	f_close((FIL*)&preset_File);
	
	//reset the lcd
	menu_repaintAll();
	
#else
#endif
};
//----------------------------------------------------
uint8_t preset_loadPattern(uint8_t presetNr)
{
#if USE_SD_CARD		
	//filename in 8.3  format
	char filename[13];
	sprintf(filename,"p%03d.pat",presetNr);
	//open the file
	FRESULT res = f_open((FIL*)&preset_File,filename,FA_OPEN_EXISTING | FA_READ);
	
	//preset_File.
	
	if(res!=FR_OK)
	{
		//file open error... maybe the file does not exist?
		return 0;
	}
	else
	{
		
		lcd_clear();
		lcd_home();
		lcd_string_F(PSTR("Loading pattern"));

		//now read the file content
		unsigned int bytesRead;
		//first the preset name
		f_read((FIL*)&preset_File,(void*)preset_currentName,8,&bytesRead);
	
	
		//then the data
#if USE_LFO		
		lfo_disable(1);
#endif		
		
			while( (frontParser_midiMsg.status != SYSEX_START))
	{
		frontPanel_sendByte(SYSEX_START);
		uart_checkAndParse();
	}		
	_delay_ms(10);
	//enter step data mode
	frontPanel_sendByte(SYSEX_SEND_STEP_DATA);
	frontPanel_sysexMode = SYSEX_SEND_STEP_DATA;
	
	/*
		//initiate the sysex mode
		frontPanel_sendByte(SYSEX_START);
		//enter send step data mode
		frontPanel_sendByte(SYSEX_SEND_STEP_DATA);
		*/
	
	uint16_t i;
	
	for(i=0;i<(STEPS_PER_PATTERN*NUM_PATTERN*NUM_TRACKS);i++)
	{
		f_read((FIL*)&preset_File,(void*)&frontParser_stepData,sizeof(StepData),&bytesRead);	
		preset_sendStepDataToSeq(i);
		//we have to give the cortex some time to cope with all the incoming data
		//since it is mainly calculating audio it takes a while to process all
		//incoming uart data
		//if((i&0x1f) == 0x1f) //every 32 steps
			_delay_us(200);
		
	}	


	//end sysex mode
	frontPanel_sendByte(SYSEX_END);
	
	
	
	
	//----- main step data	 -----
	frontParser_midiMsg.status = 0;
	//now the main step data
	while( (frontParser_midiMsg.status != SYSEX_START))
	{
		frontPanel_sendByte(SYSEX_START);
		uart_checkAndParse();
	}	
	_delay_ms(50);	
	frontPanel_sendByte(SYSEX_SEND_MAIN_STEP_DATA);
	frontPanel_sysexMode = SYSEX_SEND_MAIN_STEP_DATA;
	
	uint16_t mainStepData;
	for(i=0;i<(NUM_PATTERN*NUM_TRACKS);i++)
	{
		f_read((FIL*)&preset_File,(void*)&mainStepData,sizeof(uint16_t),&bytesRead);	
		preset_sendMainStepDataToSeq(i,mainStepData);
		//we have to give the cortex some time to cope with all the incoming data
		//since it is mainly calculating audio it takes a while to process all
		//incoming uart data
		//if((i&0x1f) == 0x1f) //every 32 steps
		_delay_us(200); //todo speed up using ACK possible?
	}				
	
	
	//end sysex mode
	frontPanel_sendByte(SYSEX_END);	
	
	//----- pattern info (next/repeat) ------

	uint8_t repeat,next;
	for(i=0;i<(NUM_PATTERN);i++)
	{
		f_read((FIL*)&preset_File,(void*)&next,sizeof(uint8_t),&bytesRead);	
		f_read((FIL*)&preset_File,(void*)&repeat,sizeof(uint8_t),&bytesRead);	

		
		frontPanel_sendData(SEQ_CC,SEQ_SET_SHOWN_PATTERN,i);
		
		frontPanel_sendData(SEQ_CC,SEQ_SET_PAT_BEAT,repeat);
		frontPanel_sendData(SEQ_CC,SEQ_SET_PAT_NEXT,next);
		//we have to give the cortex some time to cope with all the incoming data
		//since it is mainly calculating audio it takes a while to process all
		//incoming uart data
		//if((i&0x1f) == 0x1f) //every 32 steps
		_delay_us(200); //todo speed up using ACK possible?
	}				
	
	
	frontPanel_sendData(SEQ_CC,SEQ_SET_SHOWN_PATTERN,menu_shownPattern);
	
	
	
	
	//load the shuffle settings
	f_read((FIL*)&preset_File,(void*)&parameters[PAR_SHUFFLE].value,sizeof(uint8_t),&bytesRead);
	//and update on cortex
	frontPanel_sendData(SEQ_CC,SEQ_SHUFFLE,parameters[PAR_SHUFFLE].value);

	
	
	
#if USE_LFO	
	lfo_disable(0);
#endif	
		//close the file handle
		f_close((FIL*)&preset_File);
		
	}
	
	//force complete repaint
	menu_repaintAll();
	return 1;
	
#else
frontPanel_sendData(PRESET,PATTERN_LOAD,presetNr);

#endif
};
//----------------------------------------------------
//val is interpolation value between 0 and 127
uint8_t interpolate(uint8_t a, uint8_t b, uint8_t x)
{
	return ((a*255) + (b-a)*x)/255;
}
//----------------------------------------------------
/** morph parameter is 0<->128 */
void preset_morph(uint8_t morph)
{
	int i;
	//TODO so far only sound parameters are interpolated, no LFOs etc

	for(i=0;i<END_OF_SOUND_PARAMETERS;i++)
	{
		uint8_t val;
		
		val = interpolate(parameters[i].value,parameters2[i].value,morph);
		
		if(i<128) {
			frontPanel_sendData(MIDI_CC,i,val);
		} else {
			frontPanel_sendData(CC_2,i-128,val);
		}		
		
		//delay to not overflow the rx buffer on the cortex
		//TODO ACK scheme for speedup testen
		/*
		if((i&0xf) == 0xf) //every 16 steps
			_delay_ms(1);
			*/
			
		//to ommit front panel button/LED lag we have to process din dout and uart here			
		//read next button
		din_readNextInput();
		//update LEDs
		dout_updateOutputs();
		//read uart messages from sequencer
		uart_checkAndParse();
	}		
	
	/*
	//samplerate/decimation
	for(int i=0;i<6;i++)
	{
		uint8_t val;
		val = interpolate(parameters[PAR_VOICE_DECIMATION1+i].value,parameters2[PAR_VOICE_DECIMATION1+i].value,morph);
		
		//select track
		frontPanel_sendData(SEQ_CC,SEQ_SET_ACTIVE_TRACK,i);
		//send value
		frontPanel_sendData(VOICE_CC,VOICE_DECIMATION,val);
	}		
	*/
	
	
};