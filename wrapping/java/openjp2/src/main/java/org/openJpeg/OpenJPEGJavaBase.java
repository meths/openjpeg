package org.openJpeg;

public abstract class OpenJPEGJavaBase {

	public OpenJPEGJavaBase()
			throws ExceptionInInitializerError {
		
	}

	/** number of resolutions decompositions */
	protected int nbResolutions = -1;

	protected byte[] image8 = null;
	protected short[] image16 = null;
	protected int[] image24 = null;

	/** Holds the J2K compressed bytecode to decode */
	protected byte compressedStream[] = null;

	protected int width = -1;
	protected int height = -1;
	protected int depth = -1;

	/** Number of resolutions contained in the image */
	public int getNbResolutions() {
		return nbResolutions;
	}

	/** Number of resolutions contained in the image */
	public void setNbResolutions(int nbResolutions) {
		this.nbResolutions = nbResolutions;
	}

	/**
	 * Contains the decompressed version of the image, if the depth in is [9,16]
	 * bpp. Returns NULL otherwise.
	 */
	public short[] getImage16() {
		return image16;
	}

	/**
	 * Contains the decompressed version of the image, if the depth in is
	 * [17,24] bpp and the image is in color. Returns NULL otherwise.
	 */
	public int[] getImage24() {
		return image24;
	}

	/**
	 * This method must be called in depth in [9,16].
	 * 
	 * @param an
	 *            array of shorts, containing width*height values
	 */
	public void setImage16(short[] image16) {
		this.image16 = image16;
	}

	/**
	 * This method must be called in depth in [17,24] for RGB images.
	 * 
	 * @param an
	 *            array of int, containing width*height values
	 */
	public void setImage24(int[] image24) {
		this.image24 = image24;
	}

	/**
	 * This method must be called in depth in [1,8].
	 * 
	 * @param an
	 *            array of bytes, containing width*height values
	 */
	public void setImage8(byte[] image8) {
		this.image8 = image8;
	}

	/**
	 * Contains the decompressed version of the image, if the depth in is [1,8]
	 * bpp. Returns NULL otherwise.
	 */
	public byte[] getImage8() {
		return image8;
	}

	public byte[] getCompressedStream() {
		return compressedStream;
	}

	/** Image depth in bpp */
	public int getDepth() {
		return depth;
	}

	/** Image depth in bpp */
	public void setDepth(int depth) {
		this.depth = depth;
	}

	/** Image height in pixels */
	public int getHeight() {
		return height;
	}

	/** Image height in pixels */
	public void setHeight(int height) {
		this.height = height;
	}

	/** Width of the image in pixels */
	public int getWidth() {
		return width;
	}

	/** Width of the image in pixels */
	public void setWidth(int width) {
		this.width = width;
	}

	public void reset() {
		nbResolutions = -1;
		image8 = null;
		image16 = null;
		image24 = null;
		width = -1;
		height = -1;
		depth = -1;
	}

}
