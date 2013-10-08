/*
 * Copyright (c) 2002-2007, Communications and Remote Sensing Laboratory, Universite catholique de Louvain (UCL), Belgium
 * Copyright (c) 2002-2007, Professor Benoit Macq
 * Copyright (c) 2002-2007, Patrick Piscaglia, Telemis s.a.
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

package org.openJpeg;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Vector;

/**
 * This class decodes one J2K codestream into an image (width + height + depth +
 * pixels[], using the OpenJPEG.org library. To be able to log messages, the
 * called must register a IJavaJ2KDecoderLogger object.
 */
public class OpenJPEGJavaDecoder extends OpenJPEGJavaBase {
	public interface IJavaJ2KDecoderLogger {
		public void logDecoderMessage(String message);

		public void logDecoderError(String message);
	}

	private static boolean DEBUG_DECOMPRESS_FROM_BUFFER = false;
	private static boolean DEBUG_COMPRESS_FROM_BUFFER = false;	

	// ===== decompression parameters =============>
	/*
	 * These value may be changed for each image
	 */
	private String[] decoder_arguments = null;

	/** the quality layers */
	private int[] layers = null;

	private int reductionIn = 0;
	private int tileIn = -1;
	private int areaX0 = 0;
	private int areaY0 = 0;
	private int areaX1 = 0;
	private int areaY1 = 0;

	private int userChangedTile = 0;
	private int userChangedReduction = 0;
	private int userChangedArea = 0;

	private int maxTiles = 0;
	private int maxReduction = 0;
	
	private long[] segmentPositions;
	private long[] segmentLengths;

	private Vector<IJavaJ2KDecoderLogger> loggers = new Vector<IJavaJ2KDecoderLogger>();

	public void addLogger(IJavaJ2KDecoderLogger messagesAndErrorsLogger) {
		loggers.addElement(messagesAndErrorsLogger);
	}

	public void removeLogger(IJavaJ2KDecoderLogger messagesAndErrorsLogger) {
		loggers.removeElement(messagesAndErrorsLogger);
	}
	
	public void SetSegmentPositions(long[] positions)
	{
		segmentPositions = positions;
	}

	public void SetSegmentLengths(long[] lengths)
	{
		segmentLengths = lengths;
	}
	
	void alloc8() {
		if ((image8 == null || (image8 != null && image8.length != width
				* height))
				&& (depth == -1 || depth == 8)) {
			image8 = new byte[width * height];
			logMessage("OpenJPEGJavaDecoder.decompressImage: image8 length = "
					+ image8.length + " (" + width + " x " + height + ") ");
		}
	}

	void alloc16() {
		if ((image16 == null || (image16 != null && image16.length != width
				* height))
				&& (depth == -1 || depth == 16)) {
			image16 = new short[width * height];
			logMessage("OpenJPEGJavaDecoder.decompressImage: image16 length = "
					+ image16.length + " (" + width + " x " + height + ") ");
		}
	}

	void alloc24() {
		if ((image24 == null || (image24 != null && image24.length != width
				* height))
				&& (depth == -1 || depth == 24)) {
			image24 = new int[width * height];
			logMessage("OpenJPEGJavaDecoder.decompressImage: image24 length = "
					+ image24.length + " (" + width + " x " + height + ") ");
		}
	}

	int decodeJ2KtoImage() {
		return internalDecodeJ2KtoImage(convertArguments(decoder_arguments));
	}
	
	private String[] convertArguments(String[] input)
	{
		String[] arguments = new String[0 + (input != null ? input.length : 0)];
		int offset = 0;
		if (input != null)
		{
			for (int i = 0; i < input.length; i++)
				arguments[i + offset] = input[i];
		}
		return arguments;
	}
	
	//NATIVE METHODS

	/**
	 * Decode the j2k stream given in the codestream byte[] and fills the
	 * image8, image16 or image24 array, according to the bit depth.
	 */
	/* ================================================================== */
	private native int internalDecodeJ2KtoImage(String[] parameters);
	private native int internalGetDecodeFormat(String[] parameters);

	/* ================================================================== */

	public void setTileIn(int t) {
		this.tileIn = t;
	}

	public void setReductionIn(int r) {
		this.reductionIn = r;
	}

	public void setAreaIn(int x0, int y0, int x1, int y1) {
		this.areaX0 = x0;
		this.areaY0 = y0;
		this.areaX1 = x1;
		this.areaY1 = y1;
	}

	public void setUserChangedTile(int v) {
		this.userChangedTile = v;
	}

	public void setUserChangedReduction(int v) {
		this.userChangedReduction = v;
	}

	public void setUserChangedArea(int v) {
		this.userChangedArea = v;
	}

	public int getMaxTiles() {
		return maxTiles;
	}

	public int getMaxReduction() {
		return maxReduction;
	}

	public void setMaxTiles(int v) {
		this.maxTiles = v;
	}

	public void setMaxReduction(int v) {
		this.maxReduction = v;
	}

	/** Sets the codestream to be decoded */
	public void setCompressedStream(byte[] compressedStream) {
		this.compressedStream = compressedStream;
	}

	/** @return the compressed code stream length, or -1 if not defined */
	public long getCodestreamLength() {
		if (compressedStream == null)
			return -1;
		else
			return compressedStream.length;
	}

	/** This method is called either directly or by the C methods */
	public void logMessage(String message) {
		for (IJavaJ2KDecoderLogger logger : loggers)
			logger.logDecoderMessage(message);
	}

	/** This method is called either directly or by the C methods */
	public void logError(String error) {
		for (IJavaJ2KDecoderLogger logger : loggers)
			logger.logDecoderError(error);
	}

	public void reset() {
		layers = null;
		super.reset();
	}

	/** Contains all the decoding arguments other than the input/output file */
	public void setDecoderArguments(String[] argumentsForTheDecoder) {
		decoder_arguments = argumentsForTheDecoder;
	}

	public boolean canDecode(String fname)
	{
		String[] args = new String[]{fname};
		return  internalGetDecodeFormat(convertArguments(args)) == 0;
	}
	public boolean canDecode(byte[] buffer)
	{
		compressedStream = buffer;
		boolean canDecode =   internalGetDecodeFormat(convertArguments(new String[]{})) == 0;
		compressedStream = null;
		return canDecode;
	}	
	public void decode(String fname) {
		String[] args = null;
		reset();
		if (fname != null)
		{
			args = new String[1];
			if (DEBUG_DECOMPRESS_FROM_BUFFER) {
				try {
					compressedStream = getBytesFromFile(new File(fname));
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
					return;
				}

			} else {
				args[0] = fname;
			}			
		}
		decoder_arguments = args;
		decodeJ2KtoImage();
		if (DEBUG_COMPRESS_FROM_BUFFER)
		{
			OpenJPEGJavaEncoder encoder = new OpenJPEGJavaEncoder();
			if (image8 != null)
			{
				encoder.setImage8(image8);
				encoder.depth = 8;
			}
			else if (image16 != null)
			{
				encoder.setImage16(image16);
				encoder.depth = 16;				
				
			}
			else if (image24 != null)
			{
				encoder.setImage24(image24);
				encoder.depth = 24;
			}
			encoder.width = width;
			encoder.height = height;
			encoder.setNbResolutions(6);
		    encoder.encodeImageToJ2K();
		}
	}

	// Returns the contents of the file in a byte array.
	private static byte[] getBytesFromFile(File file) throws IOException {
		// Get the size of the file
		long length = file.length();

		// You cannot create an array using a long type.
		// It needs to be an int type.
		// Before converting to an int type, check
		// to ensure that file is not larger than Integer.MAX_VALUE.
		if (length > Integer.MAX_VALUE) {
			// File is too large
			throw new IOException("File is too large!");
		}

		// Create the byte array to hold the data
		byte[] bytes = new byte[(int) length];

		// Read in the bytes
		int offset = 0;
		int numRead = 0;

		InputStream is = new FileInputStream(file);
		try {
			while (offset < bytes.length
					&& (numRead = is.read(bytes, offset, bytes.length - offset)) >= 0) {
				offset += numRead;
			}
		} finally {
			is.close();
		}

		// Ensure all the bytes have been read in
		if (offset < bytes.length) {
			throw new IOException("Could not completely read file "
					+ file.getName());
		}
		return bytes;
	}

}
