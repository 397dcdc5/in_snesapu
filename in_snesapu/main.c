#define _CRT_SECURE_NO_WARNINGS
#define UNICODE
#include <stdio.h>
#include <windows.h>
#include <commctrl.h>
#include "resource.h"
#include "./sdk/xmp-sdk/xmpin.h"
#include "./sdk/spcplay-develop/snesapu.dll/SNESAPU.h"
#define REGKEYENTRYNAME "in_snesapu.dll"
#define ABOUT L"\
in_snesapu.dll is a plug-in that allows you to use \
degrade-factory improved SNESAPU.DLL (for other players) \
with XMPlay. You need to download the improved \
SNESAPU.DLL (for other players) from https://dgrfactory.jp/spcplay/ \
and place it in the same folder as in_snesapu.dll."
#define CHANNELS 2
#define SAMPLE_BIT 32
#define SAMPLE_RATE 32000
#define BFFER_SECONDS 1
#define AMP(x) ((x>65536)?(65536+((x-65536)<<1)):x)
//------------------------------
HINSTANCE InSpc,SpcApu;
unsigned char *SpcBuf=NULL,*StreamBuf=NULL,*SpcHeder256Byte=NULL;
float *StreamPos=NULL;
unsigned int StreamBufLen,StreamApuLen,PlaySeconds,EndOfPlayback,Interpolation,DspOption,Ehco,Stereo,Amp,Pitch;
static XMPFUNC_IN *xmpfin;
static XMPFUNC_MISC *xmpfmisc;
static XMPFUNC_FILE *xmpffile;
static XMPFUNC_REGISTRY *xmpfreg;
SAPUFunc ScpApuApi;
#pragma pack(push,1)
typedef struct{//silverHIRAME SAN. Plug-in for Winamp in_spc.dll v0.1.3 src:https://kurohane.net/silverhirame/
	BYTE header[33];
	BYTE unused1[2];
	BYTE id666_contain;
	BYTE version;//37
	WORD pc;
	BYTE a;
	BYTE x;
	BYTE y;
	BYTE p;
	BYTE s;
	BYTE reserved1[2];//46
	BYTE songtitle[32];
	BYTE gametitle[32];
	BYTE dumper[16];
	BYTE comments[32];
	BYTE date[11];
	BYTE length[3];
	BYTE fade[5];
	BYTE artist[32];
	BYTE channel;
	BYTE dumpby;
	BYTE reserved2[45];//256
}SPCDATA;SPCDATA *SpcHeader;
#pragma pack(pop)
////////////////////////////////////////////////////////////////DialogBox
WINAPI About(HWND hwnd){
	MessageBox(hwnd,ABOUT,L"in_snesapu.dll(x86)",MB_OK);
}
//--------------------------------------------------------------
void ScpApuApiSet(){
	ScpApuApi.SetDSPEFBCT(((int)Ehco)-32768);
	ScpApuApi.SetDSPStereo((int)Stereo);
	ScpApuApi.SetDSPAmp(AMP(Amp));
	ScpApuApi.SetDSPPitch(Pitch);
	ScpApuApi.SetAPUOpt(MIX_FLOAT,CHANNELS,-SAMPLE_BIT,SAMPLE_RATE,(Interpolation==5)?7:Interpolation,DspOption);
}
//--------------------------------------------------------------
void ConfigDlgSetCheckButton(HWND hwnd){
	CheckDlgButton(hwnd, IDC_CHECK1,(BOOL)(DspOption&DSP_ANALOG));
	CheckDlgButton(hwnd, IDC_CHECK2,(BOOL)(DspOption&DSP_OLDSMP));
	CheckDlgButton(hwnd, IDC_CHECK3,(BOOL)(DspOption&DSP_SURND));
	CheckDlgButton(hwnd, IDC_CHECK4,(BOOL)(DspOption&DSP_REVERSE));
	CheckDlgButton(hwnd, IDC_CHECK5,(BOOL)(DspOption&DSP_NOECHO));
	CheckDlgButton(hwnd, IDC_CHECK6,(BOOL)(DspOption&DSP_NOPMOD));
	CheckDlgButton(hwnd, IDC_CHECK7,(BOOL)(DspOption&DSP_NOPREAD));
	CheckDlgButton(hwnd, IDC_CHECK8,(BOOL)(DspOption&DSP_NOFIR));
	CheckDlgButton(hwnd, IDC_CHECK9,(BOOL)(DspOption&DSP_BASS));
	CheckDlgButton(hwnd,IDC_CHECK10,(BOOL)(DspOption&DSP_NOENV));
	CheckDlgButton(hwnd,IDC_CHECK11,(BOOL)(DspOption&DSP_NONOISE));
	CheckDlgButton(hwnd,IDC_CHECK12,(BOOL)(DspOption&DSP_ECHOFIR));
	CheckDlgButton(hwnd,IDC_CHECK13,(BOOL)(DspOption&DSP_NOSURND));
	CheckDlgButton(hwnd,IDC_CHECK14,(BOOL)(DspOption&DSP_ENVSPD));
	CheckDlgButton(hwnd,IDC_CHECK15,(BOOL)(DspOption&DSP_NOPLMT));
	CheckDlgButton(hwnd,IDC_CHECK16,(BOOL)(DspOption&DSP_FLOAT));
	CheckDlgButton(hwnd,IDC_CHECK17,(BOOL)(DspOption&DSP_NOSAFE));
}
//--------------------------------------------------------------
void ConfigDlgSetSlider(HWND hwnd){
	SendMessage(GetDlgItem(hwnd,IDC_SLIDER1),TBM_SETPOS,TRUE,Ehco>>8);
	SendMessage(GetDlgItem(hwnd,IDC_SLIDER2),TBM_SETPOS,TRUE,Stereo>>8);
	SendMessage(GetDlgItem(hwnd,IDC_SLIDER3),TBM_SETPOS,TRUE,(Amp>65536)?128+((Amp-65536)>>9):(Amp>>9));
}
//--------------------------------------------------------------
WINAPI CALLBACK ConfigDlg(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp){
	unsigned int n;
	unsigned short 
		*cbList1[]={L"none",L"linear",L"cubic",L"gauss",L"8-point sinc",L"4-point gauss",L"\0"},
		*cbList2[]={L"Normal",L"Old SB",L"Old ZSNES",L"\0"};
	switch(msg){
		case WM_HSCROLL:
			Ehco=  SendMessage(GetDlgItem(hwnd,IDC_SLIDER1),TBM_GETPOS,0,0)<<8;
			Stereo=SendMessage(GetDlgItem(hwnd,IDC_SLIDER2),TBM_GETPOS,0,0)<<8;
			Amp=   SendMessage(GetDlgItem(hwnd,IDC_SLIDER3),TBM_GETPOS,0,0)<<9;
			ScpApuApiSet();
			break;
		case WM_COMMAND:
			n=0;
			switch(wp&0xffff){
				case IDC_CHECK1: n=DSP_ANALOG;break;
				case IDC_CHECK2: n=DSP_OLDSMP;break;
				case IDC_CHECK3: n=DSP_SURND;break;
				case IDC_CHECK4: n=DSP_REVERSE;break;
				case IDC_CHECK5: n=DSP_NOECHO;break;
				case IDC_CHECK6: n=DSP_NOPMOD;break;
				case IDC_CHECK7: n=DSP_NOPREAD;break;
				case IDC_CHECK8: n=DSP_NOFIR;break;
				case IDC_CHECK9: n=DSP_BASS;break;
				case IDC_CHECK10:n=DSP_NOENV;break;
				case IDC_CHECK11:n=DSP_NONOISE;break;
				case IDC_CHECK12:n=DSP_ECHOFIR;break;
				case IDC_CHECK13:n=DSP_NOSURND;break;
				case IDC_CHECK14:n=DSP_ENVSPD;break;
				case IDC_CHECK15:n=DSP_NOPLMT;break;
				case IDC_CHECK16:n=DSP_FLOAT;break;
				case IDC_CHECK17:n=DSP_NOSAFE;break;
				case IDC_COMBO1:					
					if(((wp>>16)&0xffff)==CBN_SELCHANGE){
						Interpolation=SendDlgItemMessage(hwnd,IDC_COMBO1,CB_GETCURSEL,0,0);
						ScpApuApiSet();
					}
					break;
				case IDC_COMBO2:					
					if(((wp>>16)&0xffff)==CBN_SELCHANGE){
						switch(SendDlgItemMessage(hwnd,IDC_COMBO2,CB_GETCURSEL,0,0)){
							case 0:Pitch=32000;break;
							case 1:Pitch=32458;break;
							case 2:Pitch=32768;break;
						}
						ScpApuApiSet();
					}
					break;
				case IDCANCEL:
					Interpolation=INT_GAUSS;
					DspOption=0;
					Ehco=65536;
					Stereo=32768;
					Amp=65536;
					Pitch=32000;
					SendDlgItemMessage(hwnd,IDC_COMBO1,CB_SELECTSTRING,(WPARAM)Interpolation,(LPARAM)(LPCTSTR)cbList1[Interpolation]);
					switch(Pitch){
						case 32000:n=0;break;
						case 32458:n=1;break;
						case 32768:n=2;break;
					}
					SendDlgItemMessage(hwnd,IDC_COMBO2,CB_SELECTSTRING,(WPARAM)Pitch,(LPARAM)cbList2[n]);
					ConfigDlgSetCheckButton(hwnd);
					ConfigDlgSetSlider(hwnd);
					ScpApuApiSet();
					return FALSE;
			}
			if(n){
				DspOption=(DspOption&(~n))|((~DspOption)&n);
				CheckDlgButton(hwnd,wp&0xffff,(BOOL)(DspOption&n));
				ScpApuApiSet();
			}
			break;
		case WM_INITDIALOG:
			n=0;
			while(cbList1[n][0])SendDlgItemMessage(hwnd,IDC_COMBO1,CB_ADDSTRING,0,(LPARAM)cbList1[n]),n++;
			SendDlgItemMessage(hwnd,IDC_COMBO1,CB_SELECTSTRING,(WPARAM)Interpolation,(LPARAM)cbList1[Interpolation]);
			n=0;
			while(cbList2[n][0])SendDlgItemMessage(hwnd,IDC_COMBO2,CB_ADDSTRING,0,(LPARAM)cbList2[n]),n++;
			switch(Pitch){
				case 32000:n=0;break;
				case 32458:n=1;break;
				case 32768:n=2;break;
			}
			SendDlgItemMessage(hwnd,IDC_COMBO2,CB_SELECTSTRING,Pitch,(LPARAM)cbList2[n]);
			ConfigDlgSetCheckButton(hwnd);
			SendMessage(GetDlgItem(hwnd,IDC_SLIDER1),TBM_SETRANGE,0,(LPARAM)((256<<16)|(0)));
			SendMessage(GetDlgItem(hwnd,IDC_SLIDER2),TBM_SETRANGE,0,(LPARAM)((256<<16)|(0)));
			SendMessage(GetDlgItem(hwnd,IDC_SLIDER3),TBM_SETRANGE,0,(LPARAM)((256<<16)|(0)));
			SendMessage(GetDlgItem(hwnd,IDC_SLIDER1),TBM_SETPAGESIZE,0,16);
			SendMessage(GetDlgItem(hwnd,IDC_SLIDER2),TBM_SETPAGESIZE,0,16);
			SendMessage(GetDlgItem(hwnd,IDC_SLIDER3),TBM_SETPAGESIZE,0,16);
			ConfigDlgSetSlider(hwnd);
			ScpApuApiSet();
			return TRUE;
		case WM_CLOSE:
			EndDialog(hwnd,0);
			break;
	}
	return FALSE;
}
//--------------------------------------------------------------
WINAPI Config(HWND hwnd){
	DialogBox(InSpc,MAKEINTRESOURCE(IDD_DIALOG1),hwnd,ConfigDlg);
}
////////////////////////////////////////////////////////////////XMPlay
static DWORD WINAPI Process(float *buffer, DWORD count){
	float *pf1,*pf2,*pf3;
	pf3=(float*)(StreamBuf+StreamBufLen);
	if(StreamPos+count<pf3){
		__asm{
	pushad
	pushfd
	mov edi,buffer
	mov esi,StreamPos
	mov ecx,count
	cld
	rep movsd
	mov StreamPos,esi
	popfd
	popad
		}
		return count;
	}
	pf1=buffer;
	pf2=buffer+count;
	while(pf1<pf2){
		if(StreamPos>=pf3){
			ScpApuApi.EmuAPU(StreamBuf,StreamApuLen,1);
			StreamPos=(float*)StreamBuf;
			PlaySeconds+=BFFER_SECONDS*1000;
		}
		*pf1++=*StreamPos++;
	}
	if(PlaySeconds>EndOfPlayback)return 0;
	else                         return pf1-buffer;
}
//--------------------------------------------------------------
static double WINAPI GetGranularity(){
	return 0.001;// seek in millisecond units
}
//--------------------------------------------------------------
void ScpApuStreamFlash(){
	char str[2][6];	
	ScpApuApiSet();
	ScpApuApi.LoadSPCFile(SpcBuf);
	memmove(&str[0][0],SpcHeader->length,3);
	str[0][3]=0;
	memmove(&str[1][0],SpcHeader->fade,5);
	str[1][5]=0;
	ScpApuApi.SetAPULength(atoi(&str[0][0])*64000,atoi(&str[1][0])*64);
	EndOfPlayback=(atoi(&str[0][0]))*1000+atoi(&str[1][0]);
	PlaySeconds=0;
}
//--------------------------------------------------------------
static double WINAPI SetPlayPosition(DWORD pos){
	ScpApuStreamFlash();
	if(PlaySeconds>=EndOfPlayback){
		PlaySeconds=0;
		return 0;
	}
	ScpApuApi.SeekAPU((pos/1000)*64000,1);
	ScpApuApi.EmuAPU(StreamBuf,StreamApuLen,1);
	StreamPos=(float*)StreamBuf;
	PlaySeconds=(unsigned int)pos;
	return PlaySeconds/1000.0;
}
//--------------------------------------------------------------
static void WINAPI SetFormat(XMPFORMAT *form){
	form->res=4;//float
	form->chan=CHANNELS;
	form->rate=SAMPLE_RATE;
}
//--------------------------------------------------------------
static void WINAPI Close(){
}
//--------------------------------------------------------------
static DWORD WINAPI Open(const char *filename,XMPFILE file){
	int n;
	n=xmpffile->GetSize(file);
	if(SpcBuf!=NULL)free(SpcBuf);
	SpcBuf=malloc(n+66048);//guaranteed value
	xmpffile->Read(file,SpcBuf,n);
	SpcHeader=(SPCDATA*)SpcBuf;
	ScpApuStreamFlash();
	ScpApuApi.EmuAPU(StreamBuf,StreamApuLen,1);
	StreamPos=(float*)StreamBuf;
	xmpfin->SetLength((float)(EndOfPlayback/1000),TRUE);
	return 1;
}
//--------------------------------------------------------------
static BOOL WINAPI CheckFile(const char *filename, XMPFILE file){
	char buf[11],*str="SNES-SPC700";
	if(xmpffile->Read(file,buf,sizeof(buf))<sizeof(buf))return 0;
	if(memcmp(str,buf,sizeof(buf)))return 0;
	else                           return 1;
}
//--------------------------------------------------------------
static char *WINAPI GetTags(){
	return NULL;
}
//--------------------------------------------------------------
static DWORD WINAPI GetFileInfo(const char *filename, XMPFILE file, float **length, char **tags){
	SPCDATA *spcd;
	float *lens;
	char str[4];
	if(length==NULL)return 0;//There were times when nulls would come.
	if(tags==NULL)return 0;	
	if(!CheckFile(filename,file))return 0;
	xmpffile->Seek(file,0);//Returns the file read position.
	xmpffile->Read(file,SpcHeder256Byte,sizeof(SPCDATA));
	spcd=(SPCDATA*)SpcHeder256Byte;
	memmove(str,spcd->length,3);
	*(str+3)=0;
	lens=(float*)xmpfmisc->Alloc(sizeof(float)); // allocate array for length(s)
	lens[0]=(float)atoi(str);
	*length=lens;
	*tags=GetTags();
	return 1; // 1 song
}
//--------------------------------------------------------------
static void WINAPI GetInfoText(char *format, char *length){
	if(format){//format details...
		format+=sprintf(format,"SPC - ");
		format+=sprintf(format,"%dkb/s - ",(SAMPLE_BIT*SAMPLE_RATE*CHANNELS)/1000);
		sprintf(format,"%dhz",SAMPLE_RATE);
	}
}
//--------------------------------------------------------------
static void WINAPI GetGeneralInfo(char *buf){
	unsigned char str[34];
	memmove(str,SpcHeader->header,33);
	*(str+33)=0;
	buf+=sprintf(buf,"Format\t%s\r",str);
	buf+=sprintf(buf,"Bit rate\t%d kbps\r",(SAMPLE_BIT*SAMPLE_RATE*CHANNELS)/1000);
	buf+=sprintf(buf,"Sample rate\t%d hz\rChannels\t%d\rResolution\t%u bit\r",SAMPLE_RATE,CHANNELS,SAMPLE_BIT);
}
//--------------------------------------------------------------
char *GetMessageSCPFormat(char *buf,char *str,int len){
	memmove(buf,str,len);
	*(buf+len)=0;
	return buf;
}
//--------------------------------------------------------------
static void WINAPI GetMessageSCP(char *buf){
	char str[33];
	buf=xmpfmisc->FormatInfoText(buf,"Song Title", GetMessageSCPFormat(str,SpcHeader->songtitle,32));
	buf=xmpfmisc->FormatInfoText(buf,"Game Title", GetMessageSCPFormat(str,SpcHeader->gametitle,32));
	buf=xmpfmisc->FormatInfoText(buf,"Dumper",     GetMessageSCPFormat(str,SpcHeader->dumper,16));
	buf=xmpfmisc->FormatInfoText(buf,"Comment",    GetMessageSCPFormat(str,SpcHeader->comments,32));
	buf=xmpfmisc->FormatInfoText(buf,"Length(sec)",GetMessageSCPFormat(str,SpcHeader->length,3));
	buf=xmpfmisc->FormatInfoText(buf,"Fade(ms)",   GetMessageSCPFormat(str,SpcHeader->fade,5));//Probably milliseconds.
	buf=xmpfmisc->FormatInfoText(buf,"Artist",     GetMessageSCPFormat(str,SpcHeader->artist,32));
}
//--------------------------------------------------------------
static XMPIN xmpin={
	XMPIN_FLAG_CANSTREAM,// can stream from 'net, and only using XMPFILE
	"in_snesapu.dll",
	"SNES\0rsn/spc",
	About,
	Config,
	CheckFile,
	GetFileInfo,
	Open,
	Close,
	NULL,
	SetFormat,
	GetTags,
	GetInfoText,
	GetGeneralInfo,
	GetMessageSCP,
	SetPlayPosition,
	GetGranularity,
	NULL, // GetBuffering only applies when using your own file routines
	Process,
	NULL, // WriteFile, see GetBuffering
	NULL, // don't have any "Samples"
	NULL, // nor any sub-songs
	NULL,
	NULL, // GetDownloaded, see GetBuffering
};
XMPIN *WINAPI XMPIN_GetInterface(DWORD face,InterfaceProc faceproc){
	if(face!=XMPIN_FACE){//unsupported version
		static int shownerror=0;
		if(face<XMPIN_FACE&&!shownerror){
			About(NULL);
			shownerror=1;
		}
		return NULL;
	}
	xmpfin  =(XMPFUNC_IN*)      faceproc(XMPFUNC_IN_FACE  );
	xmpfmisc=(XMPFUNC_MISC*)    faceproc(XMPFUNC_MISC_FACE);
	xmpffile=(XMPFUNC_FILE*)    faceproc(XMPFUNC_FILE_FACE);
	xmpfreg =(XMPFUNC_REGISTRY*)faceproc(XMPFUNC_REGISTRY_FACE);
	Interpolation=INT_GAUSS;
	DspOption=0; //variable range           standard value
	Ehco=65536;  //-32768...+32768@        65536
	Stereo=32768;//     0...65536           32768
	Amp=65536;   //     0...65536..+uintMax 65536
	Pitch=32000; //                         32000
	xmpfreg->GetInt(REGKEYENTRYNAME,"Interpolation",&Interpolation);
	xmpfreg->GetInt(REGKEYENTRYNAME,"DspOption",&DspOption);
	xmpfreg->GetInt(REGKEYENTRYNAME,"Ehco",&Ehco);
	xmpfreg->GetInt(REGKEYENTRYNAME,"Stereo",&Stereo);
	xmpfreg->GetInt(REGKEYENTRYNAME,"Amp",&Amp);
	xmpfreg->GetInt(REGKEYENTRYNAME,"Pitch",&Pitch);
	if(SpcApu==NULL)return NULL;
	else            return &xmpin;
}
////////////////////////////////////////////////////////////////DLL MAIN
BOOL WINAPI DllMain(HINSTANCE hinst,DWORD reason,LPVOID reserved){
	switch(reason){
		case DLL_PROCESS_ATTACH:
			InSpc=hinst;
			SpcApu=LoadLibrary(L"./snesapu.dll");
			if(SpcApu!=NULL){
				memset(&ScpApuApi,0,sizeof(ScpApuApi));
				ScpApuApi.LoadSPCFile =(void*)GetProcAddress(SpcApu,"LoadSPCFile");
				ScpApuApi.SetAPUOpt   =(void*)GetProcAddress(SpcApu,"SetAPUOpt");
				ScpApuApi.SetAPULength=(void*)GetProcAddress(SpcApu,"SetAPULength");
				ScpApuApi.EmuAPU      =(void*)GetProcAddress(SpcApu,"EmuAPU");
				ScpApuApi.SeekAPU     =(void*)GetProcAddress(SpcApu,"SeekAPU");
				ScpApuApi.SetDSPEFBCT =(void*)GetProcAddress(SpcApu,"SetDSPEFBCT");
				ScpApuApi.SetDSPStereo=(void*)GetProcAddress(SpcApu,"SetDSPStereo");
				ScpApuApi.SetDSPAmp   =(void*)GetProcAddress(SpcApu,"SetDSPAmp");
				ScpApuApi.SetDSPPitch =(void*)GetProcAddress(SpcApu,"SetDSPPitch");
				ScpApuApi.SNESAPUInfo =(void*)GetProcAddress(SpcApu,"SNESAPUInfo");
				if(
					(ScpApuApi.LoadSPCFile ==NULL)||
					(ScpApuApi.SetAPUOpt   ==NULL)||
					(ScpApuApi.SetAPULength==NULL)||
					(ScpApuApi.EmuAPU      ==NULL)||
					(ScpApuApi.SeekAPU     ==NULL)||
					(ScpApuApi.SetDSPEFBCT ==NULL)||
					(ScpApuApi.SetDSPStereo==NULL)||
					(ScpApuApi.SetDSPAmp   ==NULL)||
					(ScpApuApi.SetDSPPitch ==NULL)||
					(ScpApuApi.SNESAPUInfo ==NULL)){
					FreeLibrary(SpcApu);
					SpcApu=NULL;
				}
			}
			SpcHeder256Byte=malloc(sizeof(SPCDATA));
			StreamBufLen=4*CHANNELS*SAMPLE_RATE*BFFER_SECONDS;
			StreamBuf=malloc(StreamBufLen+16);
			StreamApuLen=SAMPLE_RATE*BFFER_SECONDS;
			DisableThreadLibraryCalls(hinst);
			break;
		case DLL_PROCESS_DETACH:
			if(SpcHeder256Byte!=NULL)free(SpcHeder256Byte);
			if(SpcBuf!=NULL)free(SpcBuf);
			if(StreamBuf!=NULL)free(StreamBuf);
			if(SpcApu!=NULL)FreeLibrary(SpcApu);
			if(xmpfreg){
				xmpfreg->SetInt(REGKEYENTRYNAME,"Interpolation",&Interpolation);
				xmpfreg->SetInt(REGKEYENTRYNAME,"DspOption",&DspOption);
				xmpfreg->SetInt(REGKEYENTRYNAME,"Ehco",&Ehco);
				xmpfreg->SetInt(REGKEYENTRYNAME,"Stereo",&Stereo);
				xmpfreg->SetInt(REGKEYENTRYNAME,"Amp",&Amp);
				xmpfreg->SetInt(REGKEYENTRYNAME,"Pitch",&Pitch);
			}
			break;
	}
	return TRUE;
}