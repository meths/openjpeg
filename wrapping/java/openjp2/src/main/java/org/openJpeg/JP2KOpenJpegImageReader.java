package org.openJpeg;

import java.io.IOException;
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

import org.codecCentral.imageio.generic.GenericImageReader;

public class JP2KOpenJpegImageReader extends GenericImageReader {

	private static Logger LOGGER = Logger.getLogger("org.openJpeg.imageio-openjpeg");

	static {
		final String level = System.getProperty("org.openJpeg.imageio-openjpeg.loggerlevel");
		if (level != null && level.equalsIgnoreCase("FINE")) {
			LOGGER.setLevel(Level.FINE);
		}
	}

	private boolean isRawSource;
	private final List<JP2KCodestreamProperties> multipleCodestreams = new ArrayList<JP2KCodestreamProperties>();

	protected JP2KOpenJpegImageReader(ImageReaderSpi originatingProvider) {
		super(originatingProvider);
		
		decoder = new OpenJPEGJavaDecoder();

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
	 * Disposes all the resources, native and non, used by this
	 * {@link ImageReader} subclass.
	 */
	public void dispose() {
		super.dispose();
		if (multipleCodestreams != null) {
			multipleCodestreams.clear();
		}
	}

	/**
	 * Returns the height of a tile
	 * 
	 * @param the
	 *            index of the selected image
	 */
	public int getTileHeight(int imageIndex) throws IOException {
		checkImageIndex(imageIndex);
		final int tileHeight = multipleCodestreams.get(imageIndex).getTileHeight();
		if (LOGGER.isLoggable(Level.FINE))
			LOGGER.fine(new StringBuffer("tileHeight:").append(	Integer.toString(tileHeight)).toString());
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
		final int tileWidth = multipleCodestreams.get(imageIndex).getTileWidth();
		if (LOGGER.isLoggable(Level.FINE))
			LOGGER.fine(new StringBuffer("tileWidth:").append(Integer.toString(tileWidth)).toString());
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
		super.reset();
		dispose();
		isRawSource = false;
	}

}