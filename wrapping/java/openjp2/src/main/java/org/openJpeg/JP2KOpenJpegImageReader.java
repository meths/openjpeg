package org.openJpeg;

import java.awt.Point;
import java.awt.Transparency;
import java.awt.color.ColorSpace;
import java.awt.image.BufferedImage;
import java.awt.image.ColorModel;
import java.awt.image.ComponentColorModel;
import java.awt.image.DataBuffer;
import java.awt.image.DataBufferByte;
import java.awt.image.DataBufferInt;
import java.awt.image.DataBufferUShort;
import java.awt.image.Raster;
import java.awt.image.SampleModel;
import java.awt.image.SinglePixelPackedSampleModel;
import java.awt.image.WritableRaster;
import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.imageio.ImageReadParam;
import javax.imageio.ImageReader;
import javax.imageio.ImageTypeSpecifier;
import javax.imageio.metadata.IIOMetadata;
import javax.imageio.spi.ImageReaderSpi;
import javax.imageio.stream.ImageInputStream;

import org.codecCentral.imageio.generic.Utils;

public class JP2KOpenJpegImageReader extends ImageReader {

	private static Logger LOGGER = Logger
			.getLogger("org.openJpeg.imageio_openjpeg");

	static {
		final String level = System.getProperty("org.openJpeg.imageio_openjpeg.loggerlevel");
		if (level != null && level.equalsIgnoreCase("FINE")) {
			LOGGER.setLevel(Level.FINE);
		}
	}

	/** Size of the Temp Buffer, used when reading from an image input stream */
	private final static int TEMP_BUFFER_SIZE = 64 * 1024;

	/** The dataset input source */
	private File inputFile = null;
	
	private byte[] compressedBytes = null;

	/** The data input source name */
	private String fileName = null;

	private boolean isRawSource;

	private final List<JP2KCodestreamProperties> multipleCodestreams = new ArrayList<JP2KCodestreamProperties>();

	private int numImages = 1;

	private OpenJPEGJavaDecoder decoder;

	protected JP2KOpenJpegImageReader(ImageReaderSpi originatingProvider) {
		super(originatingProvider);
		
		decoder = new OpenJPEGJavaDecoder();

	}

	/**
	 * Checks if the specified ImageIndex is valid.
	 * 
	 * @param imageIndex
	 *            the specified imageIndex
	 * 
	 * @throws IndexOutOfBoundsException
	 *             if imageIndex is lower than 0 or if is greater than the max
	 *             number (-1) of images available within the data source
	 *             contained within the source
	 */
	protected void checkImageIndex(final int imageIndex) {
		if (imageIndex < 0 || imageIndex > numImages) {
			final StringBuffer sb = new StringBuffer(
					"Illegal imageIndex specified = ").append(imageIndex)
					.append(", while the valid imageIndex");
			if (numImages > 1)
				// There are N Images.
				sb.append(" range should be [0,").append(numImages - 1)
						.append("]!");
			else
				// Only the imageIndex 0 is valid.
				sb.append(" should be 0!");
			throw new IndexOutOfBoundsException(sb.toString());
		}
	}

	/**
	 * Returns the height in pixel of the image
	 * 
	 * @param the
	 *            index of the selected image
	 */
	public int getHeight(int imageIndex) throws IOException {
		checkImageIndex(imageIndex);
		return multipleCodestreams.get(imageIndex).getHeight();
	}

	/**
	 * Returns the width in pixel of the image
	 * 
	 * @param the
	 *            index of the selected image
	 */
	public int getWidth(int imageIndex) throws IOException {
		checkImageIndex(imageIndex);
		return multipleCodestreams.get(imageIndex).getWidth();
	}

	/**
	 * Returns an <code>IIOMetadata</code> object containing metadata associated
	 * with the given image.
	 * 
	 * @param imageIndex
	 *            the index of the image whose metadata is to be retrieved.
	 * 
	 * @return an <code>IIOMetadata</code> object.
	 * @see javax.imageio.ImageReader#getImageMetadata(int)
	 */
	public IIOMetadata getImageMetadata(int imageIndex) throws IOException {
		return new JP2KImageMetadata(multipleCodestreams.get(imageIndex));
	}

	/**
	 * Returns an <code>IIOMetadata</code> object representing the metadata
	 * associated with the input source as a whole.
	 * 
	 * @return an <code>IIOMetadata</code> object.
	 * @see javax.imageio.ImageReader#getStreamMetadata()
	 */
	public IIOMetadata getStreamMetadata() throws IOException {
		if (isRawSource)
			throw new UnsupportedOperationException(
					"Raw source detected. Actually, unable to get stream metadata");
		return new JP2KStreamMetadata();
	}

	/**
	 * Returns an <code>Iterator</code> containing possible image types to which
	 * the given image may be decoded, in the form of
	 * <code>ImageTypeSpecifiers</code>s. At least one legal image type will be
	 * returned. This implementation simply returns an
	 * <code>ImageTypeSpecifier</code> set in compliance with the property of
	 * the dataset contained within the underlying data source.
	 * 
	 * @param imageIndex
	 *            the index of the image to be retrieved.
	 * 
	 * @return an <code>Iterator</code> containing possible image types to which
	 *         the given image may be decoded, in the form of
	 *         <code>ImageTypeSpecifiers</code>s
	 * @see javax.imageio.ImageReader#getImageTypes(int)
	 */
	public Iterator<ImageTypeSpecifier> getImageTypes(int imageIndex)
			throws IOException {
		checkImageIndex(imageIndex);
		final List<ImageTypeSpecifier> l = new java.util.ArrayList<ImageTypeSpecifier>();
		final JP2KCodestreamProperties codestreamP = multipleCodestreams
				.get(imageIndex);

		// Setting SampleModel and ColorModel for the whole image
		// if (codestreamP.getColorModel() == null ||
		// codestreamP.getSampleModel() == null) {
		// try {
		// initializeSampleModelAndColorModel(codestreamP);
		// } catch (KduException kdue) {
		// throw new RuntimeException(
		// "Error while setting sample and color model", kdue);
		// }
		// }

		final ImageTypeSpecifier imageType = new ImageTypeSpecifier(
				codestreamP.getColorModel(), codestreamP.getSampleModel());
		l.add(imageType);
		return l.iterator();
	}

	/**
	 * Returns the number of images contained in the source.
	 */
	public int getNumImages(boolean allowSearch) throws IOException {
		return numImages;
	}

	/**
	 * Read the image and returns it as a complete <code>BufferedImage</code>,
	 * using a supplied <code>ImageReadParam</code>.
	 * 
	 * @param imageIndex
	 *            the index of the desired image.
	 */
	public BufferedImage read(int imageIndex, ImageReadParam param)
			throws IOException {
		checkImageIndex(imageIndex);
		if (decoder == null)
			decoder = new OpenJPEGJavaDecoder();

		decoder.decode(fileName);
		compressedBytes = null;
		decoder.setCompressedStream(null);

		int width = decoder.getWidth();
		int height = decoder.getHeight();
		int max_tiles = decoder.getMaxTiles();
		int max_reduction = decoder.getMaxReduction();

		BufferedImage bufimg=null;

		byte[] buf8;
		short[] buf16;
		int[] buf24;
		if ((buf24 = decoder.getImage24()) != null) {
			int[] bitMasks = new int[] { 0xFF0000, 0xFF00, 0xFF, 0xFF000000 };
			SinglePixelPackedSampleModel sm = new SinglePixelPackedSampleModel(
					DataBuffer.TYPE_INT, width, height, bitMasks);
			DataBufferInt db = new DataBufferInt(buf24, buf24.length);
			WritableRaster wr = Raster
					.createWritableRaster(sm, db, new Point());
			bufimg = new BufferedImage(ColorModel.getRGBdefault(), wr, false,
					null);
		} else if ((buf16 = decoder.getImage16()) != null) {
			int[] bits = { 16 };
			ColorModel cm = new ComponentColorModel(
					ColorSpace.getInstance(ColorSpace.CS_GRAY), bits, false,
					false, Transparency.OPAQUE, DataBuffer.TYPE_USHORT);

			SampleModel sm = cm.createCompatibleSampleModel(width, height);

			DataBufferUShort db = new DataBufferUShort(buf16, width * height
					* 2);

			WritableRaster ras = Raster.createWritableRaster(sm, db, null);

			bufimg = new BufferedImage(cm, ras, false, null);

		} else if ((buf8 = decoder.getImage8()) != null) {
			int[] bits = { 8 };
			ColorModel cm = new ComponentColorModel(
					ColorSpace.getInstance(ColorSpace.CS_GRAY), bits, false,
					false, Transparency.OPAQUE, DataBuffer.TYPE_BYTE);

			SampleModel sm = cm.createCompatibleSampleModel(width, height);

			DataBufferByte db = new DataBufferByte(buf8, width * height);

			WritableRaster ras = Raster.createWritableRaster(sm, db, null);

			bufimg = new BufferedImage(cm, ras, false, null);
		}
		return bufimg;
	}

	public void setInput(Object input, boolean seekForwardOnly,
			boolean ignoreMetadata) {
		reset();
		if (input == null)
			throw new NullPointerException("The provided input is null!");
		if (input instanceof File)
		{
			inputFile = (File) input;
		} else if (input instanceof byte[]) 
		{
			compressedBytes = (byte[])input;
			decoder.setCompressedStream(compressedBytes);
		} else if (input instanceof URL)
		{
			final URL tempURL = (URL) input;
			if (tempURL.getProtocol().equalsIgnoreCase("file")) {
				inputFile = Utils.urlToFile(tempURL);
			}
		} else if (input instanceof ImageInputStream) {
			try {
				
				ImageInputStream iis = (ImageInputStream)input;
				compressedBytes = new byte[(int)iis.length()];
				int bytesRead = 0;
				int offset = 0;
				while ((bytesRead = iis.read(compressedBytes,offset, TEMP_BUFFER_SIZE)) != -1)
					offset +=bytesRead;
				decoder.setCompressedStream(compressedBytes);
			} catch (IOException ioe) {
				throw new RuntimeException("Unable to read data from ImageInputStream", ioe);
			}
		}
		else if (input instanceof List)
		{
			List args = (List)input;
			if (args.size() != 3 
					|| !(args.get(0) instanceof String)
					  || !(args.get(1) instanceof long[])
					    || !(args.get(2) instanceof long[]))
			{
				throw new IllegalArgumentException("Incorrect input type!");
			}
			inputFile = new File((String)args.get(0));
			decoder.SetSegmentPositions((long[])args.get(1));
			decoder.SetSegmentLengths((long[])args.get(2));
			
		}
		else
		{
			throw new IllegalArgumentException("Incorrect input type!");
		}

		if (this.inputFile != null)
    		fileName = inputFile.getAbsolutePath();
		
		numImages = 1;
		super.setInput(input, seekForwardOnly, ignoreMetadata);
	}

	/**
	 * Disposes all the resources, native and non, used by this
	 * {@link ImageReader} subclass.
	 */
	public void dispose() {
		super.dispose();
		if (multipleCodestreams != null) {
			multipleCodestreams.clear();
		}
		numImages = 1;
	}

	/**
	 * Returns the height of a tile
	 * 
	 * @param the
	 *            index of the selected image
	 */
	public int getTileHeight(int imageIndex) throws IOException {
		checkImageIndex(imageIndex);
		final int tileHeight = multipleCodestreams.get(imageIndex)
				.getTileHeight();
		if (LOGGER.isLoggable(Level.FINE))
			LOGGER.fine(new StringBuffer("tileHeight:").append(
					Integer.toString(tileHeight)).toString());
		return tileHeight;
	}

	/**
	 * Returns the width of a tile
	 * 
	 * @param the
	 *            index of the selected image
	 */
	public int getTileWidth(int imageIndex) throws IOException {
		checkImageIndex(imageIndex);
		final int tileWidth = multipleCodestreams.get(imageIndex)
				.getTileWidth();
		if (LOGGER.isLoggable(Level.FINE))
			LOGGER.fine(new StringBuffer("tileWidth:").append(
					Integer.toString(tileWidth)).toString());
		return tileWidth;
	}


	/**
	 * Build a default {@link JP2KOpenJpegImageReadParam}
	 */
	public ImageReadParam getDefaultReadParam() {
		return new JP2KOpenJpegImageReadParam();
	}


	public int getSourceDWTLevels(int imageIndex) {
		checkImageIndex(imageIndex);
		return multipleCodestreams.get(imageIndex).getSourceDWTLevels();
	}


	public void reset() {
		super.setInput(null, false, false);
		dispose();
		numImages = -1;
		isRawSource = false;
	}

	File getInputFile() {
		return inputFile;
	}

	String getFileName() {
		return fileName;
	}
}