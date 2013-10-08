package org.openJpeg;

import java.io.File;
import java.io.IOException;
import java.util.Locale;

import javax.imageio.ImageTypeSpecifier;
import javax.imageio.ImageWriter;
import javax.imageio.spi.ImageWriterSpi;
import javax.imageio.stream.ImageOutputStream;

public class JP2KOpenJpegImageWriterSpi extends ImageWriterSpi {

    static final String[] suffixes = { "JP2", "J2C" };

    static final String[] formatNames = { "jpeg2000", "jpeg 2000", "JPEG2000", "JPEG 2000", "JP2", "JP2K" };

    static final String[] MIMETypes = { "image/jp2" };

    static final String version = "1.0";

    static final String writerCN = "org.openJpeg.imageio_openjpeg.JP2KOpenJpegImageWriterSpi";

    static final String vendorName = "CodecCentral";

    // ReaderSpiNames
    static final String[] readerSpiName = { "org.openJpeg.imageio_openjpeg.JP2KOpenJpegImageReaderSpi" };

    // StreamMetadataFormatNames and StreamMetadataFormatClassNames
    static final boolean supportsStandardStreamMetadataFormat = false;

    static final String nativeStreamMetadataFormatName = null;

    static final String nativeStreamMetadataFormatClassName = null;

    static final String[] extraStreamMetadataFormatNames = null;

    static final String[] extraStreamMetadataFormatClassNames = null;

    // ImageMetadataFormatNames and ImageMetadataFormatClassNames
    static final boolean supportsStandardImageMetadataFormat = false;

    static final String nativeImageMetadataFormatName = null;

    static final String nativeImageMetadataFormatClassName = null;

    static final String[] extraImageMetadataFormatNames = { null };

    static final String[] extraImageMetadataFormatClassNames = { null };
    
    static final Class[] OUTPUT_TYPE =  { File.class, ImageOutputStream.class };

    /**
     * Default {@link ImageWriterSpi} constructor for JP2K writers.
     */
    public JP2KOpenJpegImageWriterSpi() {
        super(vendorName, version, formatNames, suffixes, MIMETypes, writerCN,
                OUTPUT_TYPE, readerSpiName,
                supportsStandardStreamMetadataFormat,
                nativeStreamMetadataFormatName,
                nativeStreamMetadataFormatClassName,
                extraStreamMetadataFormatNames,
                extraStreamMetadataFormatClassNames,
                supportsStandardImageMetadataFormat,
                nativeImageMetadataFormatName,
                nativeImageMetadataFormatClassName,
                extraImageMetadataFormatNames,
                extraImageMetadataFormatClassNames);
    }

    /**
     * @see javax.imageio.spi.ImageWriterSpi#createWriterInstance(java.lang.Object)
     */
    public ImageWriter createWriterInstance(Object extension)
            throws IOException {
        return new JP2KOpenJpegImageWriter(this);
    }

    /**
     * @see javax.imageio.spi.IIOServiceProvider#getDescription(java.util.Locale)
     */
    public String getDescription(Locale locale) {
        return "SPI for JPEG 2000 ImageWriter based on KDU JNI";
    }

    /**
     * Refine the check if needed.
     */
    public boolean canEncodeImage(ImageTypeSpecifier type) {
//        final int numBands = type.getNumBands();
//        final int numBits = type.getBitsPerBand(0);
        return true;
    }

}
