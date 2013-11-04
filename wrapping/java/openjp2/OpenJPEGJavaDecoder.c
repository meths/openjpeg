/*
* Copyright (c) 2002-2007, Communications and Remote Sensing Laboratory, Universite catholique de Louvain (UCL), Belgium
* Copyright (c) 2002-2007, Professor Benoit Macq
* Copyright (c) 2001-2003, David Janssens
* Copyright (c) 2002-2003, Yannick Verschueren
* Copyright (c) 2003-2007, Francois-Olivier Devaux and Antonin Descampe
* Copyright (c) 2005, Herve Drolon, FreeImage Team
* Copyright (c) 2006-2007, Parvatha Elangovan
* Copyright (c) 2007, Patrick Piscaglia (Telemis)
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/
#include "opj_config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <errno.h>

#include "opj_includes.h"
#include <openjpeg.h>
#include "opj_malloc.h"

#include "org_openJpeg_OpenJPEGJavaDecoder.h"


#ifdef _WIN32
#include <windows.h>
#else
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif /* _WIN32 */

#ifdef HAVE_LIBLCMS2
#include <lcms2.h>
#endif
#ifdef HAVE_LIBLCMS1
#include <lcms.h>
#endif
#include "color.h"
#include "format_defs.h"

#define IS_READER 1

typedef struct callback_variables 
{
	JNIEnv *env;
	/** 'jclass' object used to call a Java method from the C */
	jobject *jobj;
	/** 'jclass' object used to call a Java method from the C */
	jmethodID message_mid;
	jmethodID error_mid;
} callback_variables_t;

typedef struct decode_info
{
	JNIEnv* env;	
	jobjectArray javaParameters;
	int argc;
	const char **argv;
	opj_codec_t *codec;
	opj_stream_t *stream;
	opj_image_t *image;
	jbyteArray	jba;
	jbyte		*jbBody;
	jintArray	jia;
	jint		*jiBody;
	jshortArray jsa; 
	jshort      *jsBody;
	jbyteArray	jbaCompressed;
	jbyte		*jbBodyCompressed;
	jlongArray segmentPositions;
	jlong*     bodySegmentPositions;
	OPJ_OFF_T  *opjSegmentPositions;
	jlongArray segmentLengths;
	jlong*     bodySegmentLengths;
	OPJ_SIZE_T *opjSegmentLengths;

	OPJ_BOOL   deleteImage;

} decode_info_t;

static int ext_file_format(const char *filename) 
{
	unsigned int i;

	static const char *extension[] = 
	{
		"j2k", "jp2", "jpt", "j2c", "jpc" 
	};

	static const int format[] = 
	{ 
		J2K_CFMT, JP2_CFMT, JPT_CFMT, J2K_CFMT, J2K_CFMT
	};

	char *ext = (char*)strrchr(filename, '.');

	if(ext == NULL) return -1;

	ext++;
	if(*ext) 
	{
		for(i = 0; i < sizeof(format)/sizeof(*format); i++) 
		{
			if(strnicmp(ext, extension[i], 3) == 0)
				return format[i];
		}
	}
	return -1;
}

#define JP2_RFC3745_MAGIC "\x00\x00\x00\x0c\x6a\x50\x20\x20\x0d\x0a\x87\x0a"
#define JP2_MAGIC "\x0d\x0a\x87\x0a"
/* position 45: "\xff\x52" */
#define J2K_CODESTREAM_MAGIC "\xff\x4f\xff\x51"
static const char *bar =
	"\n===========================================\n";

int get_file_format(char *filename) {
	unsigned int i;
	static const char *extension[] = {"pgx", "pnm", "pgm", "ppm", "bmp","tif", "raw", "tga", "j2k", "jp2", "jpt", "j2c" };
	static const int format[] = { PGX_DFMT, PXM_DFMT, PXM_DFMT, PXM_DFMT, BMP_DFMT, TIF_DFMT, RAW_DFMT, TGA_DFMT, J2K_CFMT, JP2_CFMT, JPT_CFMT, J2K_CFMT };
	char * ext = strrchr(filename, '.');
	if (ext == NULL)
		return -1;
	ext++;
	if(ext) {
		for(i = 0; i < sizeof(format)/sizeof(*format); i++) {
			if(strnicmp(ext, extension[i], 3) == 0) {
				return format[i];
			}
		}
	}

	return -1;
}
/**
error callback returning the message to Java andexpecting a callback_variables_t client object
*/
void error_callback(const char *msg, void *client_data) {
	jstring jbuffer;
	callback_variables_t* vars = NULL;
	JNIEnv *env = NULL;

	if (!client_data)
		return;
	vars = (callback_variables_t*) client_data;
	env = vars->env;
	
	jbuffer = (*env)->NewStringUTF(env, msg);
	(*env)->ExceptionClear(env);
	(*env)->CallVoidMethod(env, *(vars->jobj), vars->error_mid, jbuffer);

	if ((*env)->ExceptionOccurred(env)) {
		fprintf(stderr,"C: Exception during call back method\n");
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
	}
	(*env)->DeleteLocalRef(env, jbuffer);
}
/**
warning callback returning the message to Java andexpecting a callback_variables_t client object
*/
void warning_callback(const char *msg, void *client_data) {
	jstring jbuffer;
	callback_variables_t* vars = NULL;
	JNIEnv *env = NULL;

	if (!client_data)
		return;
	vars = (callback_variables_t*) client_data;
	env = vars->env;

	jbuffer = (*env)->NewStringUTF(env, msg);
	(*env)->ExceptionClear(env);
	(*env)->CallVoidMethod(env, *(vars->jobj), vars->message_mid, jbuffer);

	if ((*env)->ExceptionOccurred(env)) {
		fprintf(stderr,"C: Exception during call back method\n");
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
	}
	(*env)->DeleteLocalRef(env, jbuffer);
}
/**
information callback returning the message to Java andexpecting a callback_variables_t client object
*/
void info_callback(const char *msg, void *client_data) {
	jstring jbuffer;
	callback_variables_t* vars = NULL;
	JNIEnv *env = NULL;

	if (!client_data)
		return;
	vars = (callback_variables_t*) client_data;
	env = vars->env;

	jbuffer = (*env)->NewStringUTF(env, msg);
	(*env)->ExceptionClear(env);
	(*env)->CallVoidMethod(env, *(vars->jobj), vars->message_mid, jbuffer);

	if ((*env)->ExceptionOccurred(env)) {
		fprintf(stderr,"C: Exception during call back method\n");
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
	}
	(*env)->DeleteLocalRef(env, jbuffer);
}

static int infile_format(FILE *reader, const char *fname)
{
	const *magic_s;
	int ext_format, magic_format;
	unsigned char buf[12];

	memset(buf, 0, 12);
	fread(buf, 1, 12, reader);
	rewind(reader);

	ext_format = ext_file_format(fname);

	if(ext_format == JPT_CFMT)
		return JPT_CFMT;

	if(memcmp(buf, JP2_RFC3745_MAGIC, 12) == 0
		|| memcmp(buf, JP2_MAGIC, 4) == 0)
	{
		magic_format = JP2_CFMT; 
		magic_s = "'.jp2'";
	}
	else
		if(memcmp(buf, J2K_CODESTREAM_MAGIC, 4) == 0)
		{
			magic_format = J2K_CFMT;
			magic_s = "'.j2k' or '.jpc' or '.j2c'";
		}
		else
			return -1;

	if(magic_format == ext_format) 
		return ext_format;

	//should we log the fact that the codestream format doesn't match the file extension??

	return magic_format;
}/* infile_format() */

static int buffer_format(opj_buffer_info_t* buf_info)
{
	int magic_format;
	if (!buf_info || buf_info->len < 12)
		return -1;
	if(memcmp(buf_info->buf, JP2_RFC3745_MAGIC, 12) == 0
		|| memcmp(buf_info->buf, JP2_MAGIC, 4) == 0)
	{
		magic_format = JP2_CFMT; 
	}
	else
	{
		if(memcmp(buf_info->buf, J2K_CODESTREAM_MAGIC, 4) == 0)
		{
			magic_format = J2K_CFMT;
		}
		else
			return -1;
	}
	return magic_format;
}/*  buffer_format() */

static const char *clr_space(OPJ_COLOR_SPACE i)
{
	if(i == OPJ_CLRSPC_SRGB) return "OPJ_CLRSPC_SRGB";
	if(i == OPJ_CLRSPC_GRAY) return "OPJ_CLRSPC_GRAY";
	if(i == OPJ_CLRSPC_SYCC) return "OPJ_CLRSPC_SYCC";
	if(i == OPJ_CLRSPC_UNKNOWN) return "OPJ_CLRSPC_UNKNOWN";
	return "CLRSPC_UNDEFINED";
}

static opj_buffer_info_t fileToBuffer(const char* fileName)
{
	opj_buffer_info_t buf_info;
	FILE *reader;
	size_t len;

	memset(&buf_info, 0, sizeof(opj_buffer_info_t));
	reader = fopen(fileName, "rb");
	if (!reader)
		return buf_info;

	fseek(reader, 0, SEEK_END);
	len = (size_t)ftell(reader);
	fseek(reader, 0, SEEK_SET);

	buf_info.buf = (unsigned char*)opj_malloc(len);
	fread(buf_info.buf, 1, len, reader);

	fclose(reader);
	buf_info.cur = buf_info.buf;
	buf_info.len = len;

	return buf_info;
}

static int getDecodeFormat(const char* fileName, OPJ_OFF_T offsetToData)
{
	FILE *reader = NULL;
	int decod_format;
	if(fileName == NULL || *fileName == 0)
	{
		fprintf(stderr,"%s:%d: input file missing\n",__FILE__,__LINE__);
		return -1;
	}
	if(strlen(fileName) > OPJ_PATH_LEN - 2)
	{
		fprintf(stderr,"%s:%d: input filename too long\n",__FILE__,__LINE__);
		return -1;
	}
	reader = fopen(fileName, "rb");
	if(reader == NULL)
	{
		fprintf(stderr,"%s:%d: failed to open %s for reading\n", __FILE__,__LINE__,fileName);
		return -1;
	}
	/*-------------------------------------------------*/

	//advance to data
	if (offsetToData != 0 && OPJ_FSEEK(reader,offsetToData,SEEK_SET)) {
		return -1;
	}

	decod_format = infile_format(reader, fileName);
	fclose(reader);
	return decod_format;
}
static void release(decode_info_t *decodeInfo)
{
	/* Release the Java arguments array:*/
	if (decodeInfo->argv)
	{
		int i;
		for(i = 0; i < decodeInfo->argc; i++)
		{
			if ((decodeInfo->argv)[i] != NULL)
			{
				(*decodeInfo->env)->ReleaseStringUTFChars(decodeInfo->env, (*decodeInfo->env)->GetObjectArrayElement(decodeInfo->env, decodeInfo->javaParameters, i), (decodeInfo->argv)[i]);
			}

		}
		opj_free(decodeInfo->argv);
		decodeInfo->argv=NULL;
	}

	
	if(decodeInfo->codec) 
	{
		opj_destroy_codec(decodeInfo->codec);
		decodeInfo->codec = NULL;
	}
	
	if(decodeInfo->stream)
	{
		opj_stream_destroy_v3(decodeInfo->stream);
		decodeInfo->stream = NULL;
	}

	if(decodeInfo->deleteImage && decodeInfo->image)
	{
		opj_image_destroy(decodeInfo->image);
		decodeInfo->image = NULL;

	}
		
	if (decodeInfo->jba  && decodeInfo->jbBody )
	{
		(*decodeInfo->env)->ReleaseByteArrayElements(decodeInfo->env, decodeInfo->jba, decodeInfo->jbBody, 0);
		decodeInfo->jba =NULL;
		decodeInfo->jbBody = NULL;
	}
	if (decodeInfo->jsa && decodeInfo->jsBody )
	{
		(*decodeInfo->env)->ReleaseShortArrayElements(decodeInfo->env, decodeInfo->jsa, decodeInfo->jsBody, 0);
		decodeInfo->jsa =NULL;
		decodeInfo->jsBody = NULL;
	}
	if (decodeInfo->jia && decodeInfo->jiBody)
	{
		(*decodeInfo->env)->ReleaseIntArrayElements(decodeInfo->env, decodeInfo->jia, decodeInfo->jiBody, 0);
		decodeInfo->jia =NULL;
		decodeInfo->jiBody = NULL;
	}

	if (decodeInfo->jbaCompressed &&  decodeInfo->jbBodyCompressed)
	{
		(*decodeInfo->env)->ReleaseByteArrayElements(decodeInfo->env, decodeInfo->jbaCompressed, decodeInfo->jbBodyCompressed, 0);
		decodeInfo->jbaCompressed = NULL;
		decodeInfo->jbBodyCompressed = NULL;
	}

	if (decodeInfo->segmentPositions &&  decodeInfo->bodySegmentPositions)
	{
		(*decodeInfo->env)->ReleaseLongArrayElements(decodeInfo->env, decodeInfo->segmentPositions, decodeInfo->bodySegmentPositions, 0);
		decodeInfo->segmentPositions = NULL;
		decodeInfo->bodySegmentPositions = NULL;
	}

	if (decodeInfo->segmentLengths &&  decodeInfo->bodySegmentLengths)
	{
		(*decodeInfo->env)->ReleaseLongArrayElements(decodeInfo->env, decodeInfo->segmentLengths, decodeInfo->bodySegmentLengths, 0);
		decodeInfo->segmentLengths = NULL;
		decodeInfo->bodySegmentLengths = NULL;
	}

	if (decodeInfo->opjSegmentPositions)
	{
		opj_free(decodeInfo->opjSegmentPositions);
		decodeInfo->opjSegmentPositions = NULL;
	}

	if (decodeInfo->opjSegmentLengths)
	{
		opj_free(decodeInfo->opjSegmentLengths);
		decodeInfo->opjSegmentLengths = NULL;
	}
}

static OPJ_BOOL catchAndRelease(decode_info_t *decodeInfo)
{
	if((*decodeInfo->env)->ExceptionOccurred(decodeInfo->env))
	{
		release(decodeInfo);
		return OPJ_TRUE;
	}
	return OPJ_FALSE;

}


JNIEXPORT jint JNICALL Java_org_openJpeg_OpenJPEGJavaDecoder_internalGetDecodeFormat(JNIEnv *env, jobject obj,jobjectArray javaParameters) 
{
	int format = -1;
	int i = 0;
	jobject		object = NULL;
	jclass		klass=0;
	jboolean	isCopy = 0;
	opj_buffer_info_t buf_info;
	decode_info_t decodeInfo;
	jfieldID	fid;

	opj_file_info_t p_file_info;
	memset(&p_file_info,0, sizeof(opj_file_info_t));

	memset(&decodeInfo, 0, sizeof(decodeInfo));
	decodeInfo.env = env;
	decodeInfo.javaParameters = javaParameters;

	decodeInfo.argc = (*decodeInfo.env)->GetArrayLength(decodeInfo.env, decodeInfo.javaParameters);
	if ( catchAndRelease(&decodeInfo) == -1)
		return -1;

	//decode from buffer
	if(decodeInfo.argc == 0)
	{

		/* JNI reference to the calling class 
		*/
		klass = (*decodeInfo.env)->GetObjectClass(decodeInfo.env, obj);
		if (klass == 0)
		{
			fprintf(stderr,"GetObjectClass returned zero");
			return -1;

		}
		fid = (*decodeInfo.env)->GetFieldID(decodeInfo.env, klass,"compressedStream", "[B");
		if((*decodeInfo.env)->ExceptionOccurred(decodeInfo.env))
			return -1;

		decodeInfo.jbaCompressed = (*decodeInfo.env)->GetObjectField(decodeInfo.env, obj, fid);
		if ( catchAndRelease(&decodeInfo) == -1)
			return -1;

		if (decodeInfo.jbaCompressed != NULL)
		{
			buf_info.len = (*decodeInfo.env)->GetArrayLength(decodeInfo.env, decodeInfo.jbaCompressed);
			if ( catchAndRelease(&decodeInfo) == -1)
				return -1;

			decodeInfo.jbBodyCompressed = (*decodeInfo.env)->GetByteArrayElements(decodeInfo.env, decodeInfo.jbaCompressed, &isCopy);
			if ( catchAndRelease(&decodeInfo) == -1)
				return -1;

			buf_info.buf = (unsigned char*)decodeInfo.jbBodyCompressed;
			buf_info.cur = buf_info.buf;
			format = buffer_format(&buf_info);
		}
	}
	else if(decodeInfo.argc == 1) //decode from file
	{
		decodeInfo.argv = (const char**)opj_malloc(decodeInfo.argc*sizeof(char*));
		if(decodeInfo.argv == NULL) 
		{
			fprintf(stderr,"%s:%d: MEMORY OUT\n",__FILE__,__LINE__);
			return -1;
		}
		for(i = 0; i < decodeInfo.argc; i++) 
		{
			decodeInfo.argv[i] = NULL;
			object = (*decodeInfo.env)->GetObjectArrayElement(env, javaParameters, i);
			if ( catchAndRelease(&decodeInfo) == -1)
				return -1;
			if (object != NULL)
			{
				decodeInfo.argv[i] = (*decodeInfo.env)->GetStringUTFChars(env, object, &isCopy);
				if ( catchAndRelease(&decodeInfo) == -1)
					return -1;
			}

		}
		#ifdef DEBUG_SHOW_ARGS
		for(i = 0; i < decodeInfo.argc; i++) 
		{
			fprintf(stderr,"ARG[%i]%s\n",i,decodeInfo.argv[i]);
		}
		printf("\n");
	    #endif /* DEBUG */

		if (decodeInfo.argv && decodeInfo.argv[0] && decodeInfo.argv[0][0]!='\0')  //check for file
		{
			//check format
			format = getDecodeFormat(decodeInfo.argv[0],0);
		}

	}

	release(&decodeInfo);
	return format;

}

/* -------------------------------
* MAIN METHOD, CALLED BY JAVA 
* -----------------------------*/
JNIEXPORT jint JNICALL Java_org_openJpeg_OpenJPEGJavaDecoder_internalDecodeJ2KtoImage(JNIEnv *env, jobject obj,jobjectArray javaParameters) 
{

	opj_dparameters_t parameters;
	OPJ_BOOL hasFile = OPJ_FALSE;
	int *red, *green, *blue, *alpha;
	opj_buffer_info_t buf_info;
	int i, decod_format;
	int width, height;
	int addR, addG,addB, addA, shiftR, shiftG, shiftB, shiftA;
	OPJ_BOOL hasAlpha, fails = OPJ_FALSE;
	OPJ_CODEC_FORMAT codec_format;
	unsigned char rc, gc, bc, ac;

	/*  ==> Access variables to the Java member variables */
	jsize		arraySize;
	jclass		klass=0;
	jobject		object = NULL;
	jboolean	isCopy = 0;
	jfieldID	fid;
	jbyte		 *ptrBBody=NULL;
	jshort  *ptrSBody = NULL;
	jint		 *ptrIBody=NULL;
	callback_variables_t msgErrorCallback_vars;
	decode_info_t decodeInfo;
	opj_file_info_t* p_file_info;

	memset(&decodeInfo, 0, sizeof(decode_info_t));
	decodeInfo.env = env;
	decodeInfo.javaParameters = javaParameters;

	memset(&buf_info, 0, sizeof(opj_buffer_info_t));


	/* JNI reference to the calling class 
	*/
	klass = (*decodeInfo.env)->GetObjectClass(decodeInfo.env, obj);
	if ( catchAndRelease(&decodeInfo) == -1)
		return -1;
	if (klass == 0)
	{
		fprintf(stderr,"GetObjectClass returned zero");
		return -1;

	}

	/* Pointers to be able to call a Java method 
	* for all the info and error messages
	*/
	msgErrorCallback_vars.env = decodeInfo.env;
	msgErrorCallback_vars.jobj = &obj;
	msgErrorCallback_vars.message_mid = (*decodeInfo.env)->GetMethodID(decodeInfo.env, klass, "logMessage", "(Ljava/lang/String;)V");
	if ( catchAndRelease(&decodeInfo) == -1)
		return -1;

	msgErrorCallback_vars.error_mid = (*decodeInfo.env)->GetMethodID(decodeInfo.env, klass, "logError", "(Ljava/lang/String;)V");
	if ( catchAndRelease(&decodeInfo) == -1)
		return -1;

	
	/* Preparing the transfer of the codestream from Java to C*/
	/*printf("C: before transfering codestream\n");*/
	fid = (*decodeInfo.env)->GetFieldID(decodeInfo.env, klass,"compressedStream", "[B");
	if ( catchAndRelease(&decodeInfo) == -1)
		return -1;

	decodeInfo.jbaCompressed = (*decodeInfo.env)->GetObjectField(decodeInfo.env, obj, fid);
	if ( catchAndRelease(&decodeInfo) == -1)
		return -1;

	if (decodeInfo.jbaCompressed != NULL)
	{
		buf_info.len = (*decodeInfo.env)->GetArrayLength(decodeInfo.env, decodeInfo.jbaCompressed);
		if ( catchAndRelease(&decodeInfo) == -1)
			return -1;

		decodeInfo.jbBodyCompressed = (*decodeInfo.env)->GetByteArrayElements(decodeInfo.env, decodeInfo.jbaCompressed, &isCopy);
		if ( catchAndRelease(&decodeInfo) == -1)
			return -1;

		buf_info.buf = (unsigned char*)decodeInfo.jbBodyCompressed;
		buf_info.cur = buf_info.buf;
	}
	//if we don't have a buffer, then try to get a file name
	if (!buf_info.buf )
	{
		/* Get the String[] containing the parameters, 
		*  and converts it into a char** to simulate command line arguments.
		*/
		arraySize = (*decodeInfo.env)->GetArrayLength(decodeInfo.env, decodeInfo.javaParameters);
		if ( catchAndRelease(&decodeInfo) == -1)
			return -1;

		decodeInfo.argc = (int) arraySize;

		if(decodeInfo.argc != 1) /* program name plus input file */
		{
			fprintf(stderr,"%s:%d: input file missing\n",__FILE__,__LINE__);
			return -1;
		}
		decodeInfo.argv = (const char**)opj_malloc(decodeInfo.argc*sizeof(char*));
		if(decodeInfo.argv == NULL) 
		{
			fprintf(stderr,"%s:%d: MEMORY OUT\n",__FILE__,__LINE__);
			return -1;
		}
		for(i = 0; i < decodeInfo.argc; i++) 
		{
			decodeInfo.argv[i] = NULL;
			object = (*decodeInfo.env)->GetObjectArrayElement(decodeInfo.env, decodeInfo.javaParameters, i);
			if ( catchAndRelease(&decodeInfo) == -1)
				return -1;
			if (object != NULL)
			{
				decodeInfo.argv[i] = (*decodeInfo.env)->GetStringUTFChars(decodeInfo.env, object, &isCopy);
				if ( catchAndRelease(&decodeInfo) == -1)
					return -1;
			}

		}
	#ifdef DEBUG_SHOW_ARGS
		for(i = 0; i < decodeInfo.argc; i++) 
		{
			fprintf(stderr,"ARG[%i]%s\n",i,decodeInfo.argv[i]);
		}
		printf("\n");
	#endif /* DEBUG */
	}

	opj_set_default_decoder_parameters(&parameters);
	//extract file name and release decodeInfo.env array
	if (decodeInfo.argv && decodeInfo.argv[0] && decodeInfo.argv[0][0]!='\0')
	{
		hasFile = TRUE;
		p_file_info = (opj_file_info_t*)calloc(1, sizeof(opj_file_info_t));
		strcpy(p_file_info->infile, decodeInfo.argv[0]);

		//now check if it is segments
		/*printf("C: before transfering codestream\n");*/
		fid = (*decodeInfo.env)->GetFieldID(decodeInfo.env, klass,"segmentPositions", "[J");
		if ( catchAndRelease(&decodeInfo) == -1)
			return -1;

		decodeInfo.segmentPositions = (*decodeInfo.env)->GetObjectField(decodeInfo.env, obj, fid);
		if ( catchAndRelease(&decodeInfo) == -1)
			return -1;

		if (decodeInfo.segmentPositions != NULL)
		{
			int numPositions = 0;
			int numLengths = 0;
			int i = 0;
			OPJ_SIZE_T dataCount=0;
			OPJ_SIZE_T readCount=0;

			numPositions = (*decodeInfo.env)->GetArrayLength(decodeInfo.env, decodeInfo.segmentPositions);
			if ( catchAndRelease(&decodeInfo) == -1)
				return -1;

			decodeInfo.bodySegmentPositions = (*decodeInfo.env)->GetLongArrayElements(decodeInfo.env, decodeInfo.segmentPositions, &isCopy);
			if ( catchAndRelease(&decodeInfo) == -1)
				return -1;

			fid = (*decodeInfo.env)->GetFieldID(decodeInfo.env, klass,"segmentLengths", "[J");
			if ( catchAndRelease(&decodeInfo) == -1)
				return -1;

			decodeInfo.segmentLengths = (*decodeInfo.env)->GetObjectField(decodeInfo.env, obj, fid);
			if ( catchAndRelease(&decodeInfo) == -1)
				return -1;

			if (decodeInfo.segmentLengths != NULL)
			{

				numLengths = (*decodeInfo.env)->GetArrayLength(decodeInfo.env, decodeInfo.segmentLengths);
				if ( catchAndRelease(&decodeInfo) == -1)
					return -1;

				decodeInfo.bodySegmentLengths = (*decodeInfo.env)->GetLongArrayElements(decodeInfo.env, decodeInfo.segmentLengths, &isCopy);
				if ( catchAndRelease(&decodeInfo) == -1)
					return -1;
			}
			if (numPositions == 0 || numLengths == 0 || numPositions != numLengths)
			{
				release(&decodeInfo);
				return -1;
			}

			decodeInfo.opjSegmentPositions = (OPJ_OFF_T*)opj_malloc(numPositions * sizeof(OPJ_OFF_T));
			for (i = 0; i < numPositions; ++i)
			{
				decodeInfo.opjSegmentPositions[i] = decodeInfo.bodySegmentPositions[i];
			}

			decodeInfo.opjSegmentLengths =  (OPJ_SIZE_T*)opj_malloc(numLengths*sizeof(OPJ_SIZE_T));
			for (i = 0; i < numLengths; ++i)
			{
				decodeInfo.opjSegmentLengths[i] = decodeInfo.bodySegmentLengths[i];
				p_file_info->dataLength += decodeInfo.bodySegmentLengths[i];
			}

			p_file_info->numSegmentsMinusOne = numPositions-1;
			p_file_info->p_segmentPositionsList = decodeInfo.opjSegmentPositions;
			p_file_info->p_segmentLengths = decodeInfo.opjSegmentLengths;
		}
	}

	if (hasFile)
	{
		OPJ_OFF_T offsetToData = 0;
		if (p_file_info->p_segmentPositionsList != NULL)
			offsetToData = p_file_info->p_segmentPositionsList[0];
		decod_format = getDecodeFormat( p_file_info->infile, offsetToData);
	
	}
	else
	{
		/* Preparing the transfer of the codestream from Java to C*/
		/*printf("C: before transfering codestream\n");*/
		fid = (*decodeInfo.env)->GetFieldID(decodeInfo.env, klass,"compressedStream", "[B");
		if ( catchAndRelease(&decodeInfo) == -1)
			return -1;

		decodeInfo.jbaCompressed = (*decodeInfo.env)->GetObjectField(decodeInfo.env, obj, fid);
		if ( catchAndRelease(&decodeInfo) == -1)
			return -1;

		if (decodeInfo.jbaCompressed != NULL)
		{
			buf_info.len = (*decodeInfo.env)->GetArrayLength(decodeInfo.env, decodeInfo.jbaCompressed);
			if ( catchAndRelease(&decodeInfo) == -1)
				return -1;

			decodeInfo.jbBodyCompressed = (*decodeInfo.env)->GetByteArrayElements(decodeInfo.env, decodeInfo.jbaCompressed, &isCopy);
			if ( catchAndRelease(&decodeInfo) == -1)
				return -1;

			buf_info.buf = (unsigned char*)decodeInfo.jbBodyCompressed;
		}
		if (!buf_info.buf )
		{
			release(&decodeInfo);
			return -1;
		}
		buf_info.cur = buf_info.buf;
		decod_format = buffer_format(&buf_info);
	}
	if(decod_format == -1) 
	{
		fprintf(stderr,"%s:%d: decode format missing\n",__FILE__,__LINE__);
		release(&decodeInfo);
		return -1;
	}

	/*-----------------------------------------------*/
	if(decod_format == J2K_CFMT)
		codec_format = OPJ_CODEC_J2K;
	else
		if(decod_format == JP2_CFMT)
			codec_format = OPJ_CODEC_JP2;
		else
			if(decod_format == JPT_CFMT)
				codec_format = OPJ_CODEC_JPT;
			else
			{
				/* clarified in infile_format() : */
				release(&decodeInfo);
				return -1;
			}
			parameters.decod_format = decod_format;
			while(1)
			{
				int tile_index=-1, user_changed_tile, user_changed_reduction;
				int max_tiles, max_reduction;
				fails = OPJ_TRUE;
				if (hasFile)
				{
				    decodeInfo.stream = opj_stream_create_default_file_stream_v4(p_file_info,1);	
				}
				else
				{
				    decodeInfo.stream =  opj_stream_create_buffer_stream(&buf_info, 1); 
				}

				if(decodeInfo.stream == NULL) 
				{
					fprintf(stderr,"%s:%d: NO decodeInfo.stream\n",__FILE__,__LINE__);	
					break;
				}
				decodeInfo.codec = opj_create_decompress(codec_format);
				if(decodeInfo.codec == NULL) 
				{
					fprintf(stderr,"%s:%d: NO coded\n",__FILE__,__LINE__);
					break;
				}

				opj_set_info_handler(decodeInfo.codec, error_callback, &msgErrorCallback_vars);
				opj_set_info_handler(decodeInfo.codec, warning_callback, &msgErrorCallback_vars);
				opj_set_info_handler(decodeInfo.codec, info_callback, &msgErrorCallback_vars);

				if( !opj_setup_decoder(decodeInfo.codec, &parameters)) 
				{
					fprintf(stderr,"%s:%d:\n\topj_setup_decoder failed\n",__FILE__,__LINE__);
					break;
				}

				fid = (*decodeInfo.env)->GetFieldID(decodeInfo.env, klass,"userChangedTile", "I");
				if ( catchAndRelease(&decodeInfo) == -1)
					return -1;

				user_changed_tile = (int) (*decodeInfo.env)->GetIntField(decodeInfo.env, obj, fid);
				if ( catchAndRelease(&decodeInfo) == -1)
					return -1;

				fid = (*decodeInfo.env)->GetFieldID(decodeInfo.env, klass,"userChangedReduction", "I");
				if ( catchAndRelease(&decodeInfo) == -1)
					return -1;

				user_changed_reduction = (int) (*decodeInfo.env)->GetIntField(decodeInfo.env, obj, fid);
				if ( catchAndRelease(&decodeInfo) == -1)
					return -1;

				if(user_changed_tile && user_changed_reduction)
				{
					int reduction;
					fid = (*decodeInfo.env)->GetFieldID(decodeInfo.env, klass,"tileIn", "I");
					if ( catchAndRelease(&decodeInfo) == -1)
						return -1;

					tile_index = (int) (*decodeInfo.env)->GetIntField(decodeInfo.env, obj, fid);
					if ( catchAndRelease(&decodeInfo) == -1)
						return -1;

					fid = (*decodeInfo.env)->GetFieldID(decodeInfo.env, klass,"reductionIn", "I");
					if ( catchAndRelease(&decodeInfo) == -1)
						return -1;

					reduction = (int) (*decodeInfo.env)->GetIntField(decodeInfo.env, obj, fid);
					if ( catchAndRelease(&decodeInfo) == -1)
						return -1;

					opj_set_decoded_resolution_factor(decodeInfo.codec, reduction);
				}

				if( !opj_read_header(decodeInfo.stream, decodeInfo.codec, &decodeInfo.image))
				{
					fprintf(stderr,"%s:%d:\n\topj_read_header failed\n",__FILE__,__LINE__);
					break;
				}

				fid = (*decodeInfo.env)->GetFieldID(decodeInfo.env, klass,"maxTiles", "I");
				if ( catchAndRelease(&decodeInfo) == -1)
					return -1;

				max_tiles = (int) (*decodeInfo.env)->GetIntField(decodeInfo.env, obj, fid);
				if ( catchAndRelease(&decodeInfo) == -1)
					return -1;

				fid = (*decodeInfo.env)->GetFieldID(decodeInfo.env, klass,"maxReduction", "I");
				if ( catchAndRelease(&decodeInfo) == -1)
					return -1;

				max_reduction = (int) (*decodeInfo.env)->GetIntField(decodeInfo.env, obj, fid);
				if ( catchAndRelease(&decodeInfo) == -1)
					return -1;

				if( !(user_changed_tile && user_changed_reduction)
					|| (max_tiles <= 0) || (max_reduction <= 0) )
				{
					opj_codestream_info_v2_t *cstr;

					cstr = opj_get_cstr_info(decodeInfo.codec);

					max_reduction = cstr->m_default_tile_info.tccp_info->numresolutions;
					max_tiles = cstr->tw * cstr->th;

					//    FLImage_put_max_tile_and_reduction(max_tiles, max_factor);
					fid = (*decodeInfo.env)->GetFieldID(decodeInfo.env, klass,"maxTiles", "I");
					if ( catchAndRelease(&decodeInfo) == -1)
						return -1;

					(*decodeInfo.env)->SetIntField(decodeInfo.env, obj, fid, max_tiles);
					if ( catchAndRelease(&decodeInfo) == -1)
						return -1;

					fid = (*decodeInfo.env)->GetFieldID(decodeInfo.env, klass,"maxReduction", "I");
					if ( catchAndRelease(&decodeInfo) == -1)
						return -1;

					(*decodeInfo.env)->SetIntField(decodeInfo.env, obj, fid, max_reduction);
					if ( catchAndRelease(&decodeInfo) == -1)
						return -1;

					fid = (*decodeInfo.env)->GetFieldID(decodeInfo.env, klass,"userChangedTile", "I");
					if ( catchAndRelease(&decodeInfo) == -1)
						return -1;

					(*decodeInfo.env)->SetIntField(decodeInfo.env, obj, fid, 1);
					if ( catchAndRelease(&decodeInfo) == -1)
						return -1;

					fid = (*decodeInfo.env)->GetFieldID(decodeInfo.env, klass,"userChangedReduction", "I");
					if ( catchAndRelease(&decodeInfo) == -1)
						return -1;

					(*decodeInfo.env)->SetIntField(decodeInfo.env, obj, fid, 1);
					if ( catchAndRelease(&decodeInfo) == -1)
						return -1;
				}

				if(tile_index < 0)
				{
					unsigned int x0, y0, x1, y1;
					int user_changed_area;

					x0 = y0 = x1 = y1 = 0;

					fid = (*decodeInfo.env)->GetFieldID(decodeInfo.env, klass,"userChangedArea", "I");
					if ( catchAndRelease(&decodeInfo) == -1)
						return -1;

					user_changed_area = (int) (*decodeInfo.env)->GetIntField(decodeInfo.env, obj, fid);
					if ( catchAndRelease(&decodeInfo) == -1)
						return -1;

					if(user_changed_area)
					{
						fid = (*decodeInfo.env)->GetFieldID(decodeInfo.env, klass,"areaX0", "I");
						if ( catchAndRelease(&decodeInfo) == -1)
							return -1;

						x0 = (unsigned int) (*decodeInfo.env)->GetIntField(decodeInfo.env, obj, fid);
						if ( catchAndRelease(&decodeInfo) == -1)
							return -1;

						fid = (*decodeInfo.env)->GetFieldID(decodeInfo.env, klass,"areaY0", "I");
						if ( catchAndRelease(&decodeInfo) == -1)
							return -1;

						y0 = (unsigned int) (*decodeInfo.env)->GetIntField(decodeInfo.env, obj, fid);
						if ( catchAndRelease(&decodeInfo) == -1)
							return -1;

						fid = (*decodeInfo.env)->GetFieldID(decodeInfo.env, klass,"areaX1", "I");
						if ( catchAndRelease(&decodeInfo) == -1)
							return -1;

						x1 = (unsigned int) (*decodeInfo.env)->GetIntField(decodeInfo.env, obj, fid);
						if ( catchAndRelease(&decodeInfo) == -1)
							return -1;

						fid = (*decodeInfo.env)->GetFieldID(decodeInfo.env, klass,"areaY1", "I");
						if ( catchAndRelease(&decodeInfo) == -1)
							return -1;

						y1 = (unsigned int) (*decodeInfo.env)->GetIntField(decodeInfo.env, obj, fid);
						if ( catchAndRelease(&decodeInfo) == -1)
							return -1;
					}

					if( !opj_set_decode_area(decodeInfo.codec, decodeInfo.image, x0, y0, x1, y1))
					{
						fprintf(stderr,"%s:%d:\n\topj_set_decode_area failed\n",__FILE__,__LINE__);
						break;
					}
					if( !opj_decode(decodeInfo.codec, decodeInfo.stream, decodeInfo.image))
					{
						fprintf(stderr,"%s:%d:\n\topj_decode failed\n",__FILE__,__LINE__);
						break;
					}
				}	/* if(tile_index < 0) */
				else
				{
					if( !opj_get_decoded_tile(decodeInfo.codec, decodeInfo.stream, decodeInfo.image, tile_index))
					{
						fprintf(stderr,"%s:%d:\n\topj_get_decoded_tile failed\n",__FILE__,__LINE__);
						break;
					}
				}

				if( !opj_end_decompress(decodeInfo.codec, decodeInfo.stream))
				{
					fprintf(stderr,"%s:%d:\n\topj_end_decompress failed\n",__FILE__,__LINE__);
					break;
				}

				fails = OPJ_FALSE;
				break;

			}
			decodeInfo.deleteImage = fails;
			release(&decodeInfo);
			if(fails)
			{
				return -1;
			}
			decodeInfo.deleteImage = TRUE;

			if(decodeInfo.image->color_space != OPJ_CLRSPC_SYCC
				&& decodeInfo.image->numcomps == 3
				&& decodeInfo.image->comps[0].dx == decodeInfo.image->comps[0].dy
				&& decodeInfo.image->comps[1].dx != 1)
				decodeInfo.image->color_space = OPJ_CLRSPC_SYCC;
			else
				if(decodeInfo.image->numcomps <= 2)
					decodeInfo.image->color_space = OPJ_CLRSPC_GRAY;

			if(decodeInfo.image->color_space == OPJ_CLRSPC_SYCC)
			{
				color_sycc_to_rgb(decodeInfo.image);
			}
			if(decodeInfo.image->icc_profile_buf)
			{
#if defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)
				color_apply_icc_profile(decodeInfo.image);
#endif

				opj_free(decodeInfo.image->icc_profile_buf);
				decodeInfo.image->icc_profile_buf = NULL;
				decodeInfo.image->icc_profile_len = 0;
			}

			width = decodeInfo.image->comps[0].w;
			height = decodeInfo.image->comps[0].h;
			/* Set JAVA width and height:
			*/
			fid = (*decodeInfo.env)->GetFieldID(decodeInfo.env, klass, "width", "I");
			if ( catchAndRelease(&decodeInfo) == -1)
				return -1;

			(*decodeInfo.env)->SetIntField(decodeInfo.env, obj, fid, width);
			if ( catchAndRelease(&decodeInfo) == -1)
				return -1;

			fid = (*decodeInfo.env)->GetFieldID(decodeInfo.env, klass, "height", "I");
			if ( catchAndRelease(&decodeInfo) == -1)
				return -1;

			(*decodeInfo.env)->SetIntField(decodeInfo.env, obj, fid, height);
			if ( catchAndRelease(&decodeInfo) == -1)
				return -1;


			fid = (*decodeInfo.env)->GetFieldID(decodeInfo.env, klass, "bitsPerSample", "I");
			if ( catchAndRelease(&decodeInfo) == -1)
				return -1;

			(*decodeInfo.env)->SetIntField(decodeInfo.env, obj, fid, decodeInfo.image->comps[0].prec);
			if ( catchAndRelease(&decodeInfo) == -1)
				return -1;

			if ((decodeInfo.image->numcomps >= 3
				&& decodeInfo.image->comps[0].dx == decodeInfo.image->comps[1].dx
				&& decodeInfo.image->comps[1].dx == decodeInfo.image->comps[2].dx
				&& decodeInfo.image->comps[0].dy == decodeInfo.image->comps[1].dy
				&& decodeInfo.image->comps[1].dy == decodeInfo.image->comps[2].dy
				&& decodeInfo.image->comps[0].prec == decodeInfo.image->comps[1].prec
				&& decodeInfo.image->comps[1].prec == decodeInfo.image->comps[2].prec
				)/* RGB[A] */
				||
				(decodeInfo.image->numcomps == 2
				&& decodeInfo.image->comps[0].dx == decodeInfo.image->comps[1].dx
				&& decodeInfo.image->comps[0].dy == decodeInfo.image->comps[1].dy
				&& decodeInfo.image->comps[0].prec == decodeInfo.image->comps[1].prec
				)
				) /* GA */
			{
				jmethodID mid;	
				int pix, has_alpha4, has_alpha2, has_rgb;

				shiftA = shiftR = shiftG = shiftB = 0;
				addA = addR = addG = addB = 0;
				alpha = NULL;

				has_rgb = (decodeInfo.image->numcomps == 3);
				has_alpha4 = (decodeInfo.image->numcomps == 4);
				has_alpha2 = (decodeInfo.image->numcomps == 2);
				hasAlpha = (has_alpha4 || has_alpha2);

				if(has_rgb)
				{
					if(decodeInfo.image->comps[0].prec > 8)
						shiftR = decodeInfo.image->comps[0].prec - 8;

					if(decodeInfo.image->comps[1].prec > 8)
						shiftG = decodeInfo.image->comps[1].prec - 8;

					if(decodeInfo.image->comps[2].prec > 8)
						shiftB = decodeInfo.image->comps[2].prec - 8;

					if(decodeInfo.image->comps[0].sgnd)
						addR = (1 << (decodeInfo.image->comps[0].prec - 1));

					if(decodeInfo.image->comps[1].sgnd)
						addG = (1 << (decodeInfo.image->comps[1].prec - 1));

					if(decodeInfo.image->comps[2].sgnd)
						addB = (1 << (decodeInfo.image->comps[2].prec - 1));

					red = decodeInfo.image->comps[0].data;
					green = decodeInfo.image->comps[1].data;
					blue = decodeInfo.image->comps[2].data;

					if(has_alpha4)
					{
						alpha = decodeInfo.image->comps[3].data;

						if(decodeInfo.image->comps[3].prec > 8)
							shiftA = decodeInfo.image->comps[3].prec - 8;

						if(decodeInfo.image->comps[3].sgnd)
							addA = (1 << (decodeInfo.image->comps[3].prec - 1));
					}

				}	/* if(has_rgb) */
				else
				{
					if(decodeInfo.image->comps[0].prec > 8)
						shiftR = decodeInfo.image->comps[0].prec - 8;

					if(decodeInfo.image->comps[0].sgnd)
						addR = (1 << (decodeInfo.image->comps[0].prec - 1));

					red = green = blue = decodeInfo.image->comps[0].data;

					if(has_alpha2)
					{
						alpha = decodeInfo.image->comps[1].data;

						if(decodeInfo.image->comps[1].prec > 8)
							shiftA = decodeInfo.image->comps[1].prec - 8;

						if(decodeInfo.image->comps[1].sgnd)
							addA = (1 << (decodeInfo.image->comps[1].prec - 1));
					}
				}	/* if(has_rgb) */

				/* Allocate JAVA memory:
				*/
				mid = (*decodeInfo.env)->GetMethodID(decodeInfo.env, klass, "alloc24", "()V");
				if ( catchAndRelease(&decodeInfo) == -1)
					return -1;

				(*decodeInfo.env)->CallVoidMethod(decodeInfo.env, obj, mid);
				if ( catchAndRelease(&decodeInfo) == -1)
					return -1;

				ac = 255;/* 255: FULLY_OPAQUE; 0: FULLY_TRANSPARENT */

				/* Get the pointer to the Java structure where the data must be copied
				*/
				fid = (*decodeInfo.env)->GetFieldID(decodeInfo.env, klass,"image24", "[I");
				if ( catchAndRelease(&decodeInfo) == -1)
					return -1;

				decodeInfo.jia = (*decodeInfo.env)->GetObjectField(decodeInfo.env, obj, fid);
				if ( catchAndRelease(&decodeInfo) == -1)
					return -1;

				decodeInfo.jiBody = (*decodeInfo.env)->GetIntArrayElements(decodeInfo.env, decodeInfo.jia, 0);
				if ( catchAndRelease(&decodeInfo) == -1)
					return -1;
				ptrIBody = decodeInfo.jiBody;

				for(i = 0; i < width*height; i++)
				{
					pix = addR + *red++;

					if(shiftR)
					{
						pix = ((pix>>shiftR)+((pix>>(shiftR-1))%2));
						if(pix > 255) pix = 255; else if(pix < 0) pix = 0;
					}
					rc = (unsigned char) pix;

					pix = addG + *green++;

					if(shiftG)
					{
						pix = ((pix>>shiftG)+((pix>>(shiftG-1))%2));
						if(pix > 255) pix = 255; else if(pix < 0) pix = 0;
					}
					gc = (unsigned char)pix;

					pix = addB + *blue++;

					if(shiftB)
					{
						pix = ((pix>>shiftB)+((pix>>(shiftB-1))%2));
						if(pix > 255) pix = 255; else if(pix < 0) pix = 0;
					}
					bc = (unsigned char)pix;

					if(hasAlpha)
					{
						pix = addA + *alpha++;

						if(shiftA)
						{
							pix = ((pix>>shiftA)+((pix>>(shiftA-1))%2));
							if(pix > 255) pix = 255; else if(pix < 0) pix = 0;
						}
						ac = (unsigned char)pix;
					}
					/*                         A        R          G       B
					*/
					*ptrIBody++ = (int)((ac<<24) | (rc<<16) | (gc<<8) | bc);

				}	/* for(i) */
				/* Replace image24 buffer: 
				*/

			}/* if (decodeInfo.image->numcomps >= 3  */ 
			else
				if(decodeInfo.image->numcomps == 1) /* Grey */
				{
					/* 1 component 8 or 16 bpp decodeInfo.image
					*/
					int *grey = decodeInfo.image->comps[0].data;
					if(decodeInfo.image->comps[0].prec <= 8) 
					{
						jmethodID mid;

						/* Allocate JAVA memory:
						*/
						mid = (*decodeInfo.env)->GetMethodID(decodeInfo.env, klass, "alloc8", "()V");
						if ( catchAndRelease(&decodeInfo) == -1)
							return -1;

						(*decodeInfo.env)->CallVoidMethod(decodeInfo.env, obj, mid);
						if ( catchAndRelease(&decodeInfo) == -1)
							return -1;

						fid = (*decodeInfo.env)->GetFieldID(decodeInfo.env, klass,"image8", "[B");
						if ( catchAndRelease(&decodeInfo) == -1)
							return -1;

						decodeInfo.jba = (*decodeInfo.env)->GetObjectField(decodeInfo.env, obj, fid);
						if ( catchAndRelease(&decodeInfo) == -1)
							return -1;

						decodeInfo.jbBody = (*decodeInfo.env)->GetByteArrayElements(decodeInfo.env, decodeInfo.jba, 0);
						if ( catchAndRelease(&decodeInfo) == -1)
							return -1;

						ptrBBody = decodeInfo.jbBody;
						for(i=0; i<width*height; i++) 
						{
							*ptrBBody++ = *grey++;
						}
						/* Replace image8 buffer:
						*/
					} 
					else /* prec[9:16] */
					{
						jmethodID mid;
						int *grey;
						int v, ushift = 0, dshift = 0, force16 = 0;

						grey = decodeInfo.image->comps[0].data;
						/* Allocate JAVA memory:
						*/
						mid = (*decodeInfo.env)->GetMethodID(decodeInfo.env, klass, "alloc16", "()V");
						if ( catchAndRelease(&decodeInfo) == -1)
							return -1;

						(*decodeInfo.env)->CallVoidMethod(decodeInfo.env, obj, mid);
						if ( catchAndRelease(&decodeInfo) == -1)
							return -1;

						fid = (*decodeInfo.env)->GetFieldID(decodeInfo.env, klass,"image16", "[S");
						if ( catchAndRelease(&decodeInfo) == -1)
							return -1;

						decodeInfo.jsa = (*decodeInfo.env)->GetObjectField(decodeInfo.env, obj, fid);
						if ( catchAndRelease(&decodeInfo) == -1)
							return -1;

						decodeInfo.jsBody = (*decodeInfo.env)->GetShortArrayElements(decodeInfo.env, decodeInfo.jsa, 0);
						if ( catchAndRelease(&decodeInfo) == -1)
							return -1;

						ptrSBody = decodeInfo.jsBody;

						for(i=0; i<width*height; i++) 
						{
							//disable shift up for signed data: don't know why we are doing this
							*ptrSBody++ = *grey++;
						}
						/* Replace image16 buffer:
						*/
					}
				}	
				else 
				{
					int *grey;

					fputs(bar, stderr);
					fprintf(stderr,"%s:%d:Can show only first component of decodeInfo.image\n"
						"  components(%d) prec(%d) color_space[%d](%s)\n"
						"  RECT(%d,%d,%d,%d)\n",__FILE__,__LINE__,decodeInfo.image->numcomps,
						decodeInfo.image->comps[0].prec,
						decodeInfo.image->color_space,clr_space(decodeInfo.image->color_space),
						decodeInfo.image->x0,decodeInfo.image->y0,decodeInfo.image->x1,decodeInfo.image->y1 );

					for(i = 0; i < decodeInfo.image->numcomps; ++i)
					{
						fprintf(stderr,"[%d]dx(%d) dy(%d) w(%d) h(%d) signed(%u)\n",i,
							decodeInfo.image->comps[i].dx ,decodeInfo.image->comps[i].dy,
							decodeInfo.image->comps[i].w,decodeInfo.image->comps[i].h,
							decodeInfo.image->comps[i].sgnd);
					}
					fputs(bar, stderr);

					/* 1 component 8 or 16 bpp decodeInfo.image
					*/
					grey = decodeInfo.image->comps[0].data;
					if(decodeInfo.image->comps[0].prec <= 8) 
					{
						jmethodID mid;

						/* Allocate JAVA memory:
						*/
						mid = (*decodeInfo.env)->GetMethodID(decodeInfo.env, klass, "alloc8", "()V");
						if ( catchAndRelease(&decodeInfo) == -1)
							return -1;

						(*decodeInfo.env)->CallVoidMethod(decodeInfo.env, obj, mid);
						if ( catchAndRelease(&decodeInfo) == -1)
							return -1;

						fid = (*decodeInfo.env)->GetFieldID(decodeInfo.env, klass,"image8", "[B");
						if ( catchAndRelease(&decodeInfo) == -1)
							return -1;

						decodeInfo.jba = (*decodeInfo.env)->GetObjectField(decodeInfo.env, obj, fid);
						if ( catchAndRelease(&decodeInfo) == -1)
							return -1;

						decodeInfo.jbBody = (*decodeInfo.env)->GetByteArrayElements(decodeInfo.env, decodeInfo.jba, 0);
						if ( catchAndRelease(&decodeInfo) == -1)
							return -1;

						ptrBBody = decodeInfo.jbBody;
						for(i=0; i<width*height; i++) 
						{
							*ptrBBody++ = *grey++;
						}
						/* Replace image8 buffer:
						*/
					} 
					else /* prec[9:16] */
					{
						jmethodID mid;
						int *grey;
						int v, ushift = 0, dshift = 0, force16 = 0;

						grey = decodeInfo.image->comps[0].data;

						/* Allocate JAVA memory:
						*/
						mid = (*decodeInfo.env)->GetMethodID(decodeInfo.env, klass, "alloc16", "()V");
						if ( catchAndRelease(&decodeInfo) == -1)
							return -1;

						(*decodeInfo.env)->CallVoidMethod(decodeInfo.env, obj, mid);
						if ( catchAndRelease(&decodeInfo) == -1)
							return -1;

						fid = (*decodeInfo.env)->GetFieldID(decodeInfo.env, klass,"image16", "[S");
						if ( catchAndRelease(&decodeInfo) == -1)
							return -1;

						decodeInfo.jsa = (*decodeInfo.env)->GetObjectField(decodeInfo.env, obj, fid);
						if ( catchAndRelease(&decodeInfo) == -1)
							return -1;

						decodeInfo.jsBody = (*decodeInfo.env)->GetShortArrayElements(decodeInfo.env, decodeInfo.jsa, 0);
						if ( catchAndRelease(&decodeInfo) == -1)
							return -1;

						ptrSBody = decodeInfo.jsBody;

						for(i=0; i<width*height; i++) 
						{
							*ptrSBody++ = *grey++;
						}
						/* Replace image16 buffer:
						*/
					}
				}
				release(&decodeInfo);
				if(fails)
					return -1;

				return 0; /* OK */
} /* Java_OpenJPEGJavaDecoder_internalDecodeJ2KtoImage() */

/* end OpenJPEGJavaDecoder.c */

