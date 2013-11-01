package org.openJpeg;



import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.util.Arrays;
import java.util.List;
import java.util.Locale;

import javax.imageio.ImageReader;
import javax.imageio.stream.ImageInputStream;

import org.codecCentral.imageio.generic.DecoderBase;
import org.codecCentral.imageio.generic.GenericImageReaderSpi;

public class JP2KOpenJpegImageReaderSpi extends GenericImageReaderSpi {

  
    static final String[] suffixes = { "jp2", "jp2k", "j2k", "j2c" };
     
    static final String[] formatNames = { "jpeg2000", "jpeg 2000", "JPEG 2000", "JPEG2000" };
     
    static final String[] MIMETypes = { "image/jp2", "image/jp2k", "image/j2k", "image/j2c" };

    static final String version = "1.0";

    static final String readerCN = "org.openJpeg.imageio_openjpeg.JP2KOpenJpegImageReader";

    static final String vendorName = "CodecCentral";

    // writerSpiNames
    static final String[] wSN = { null };

    // StreamMetadataFormatNames and StreamMetadataFormatClassNames
    static final boolean supportsStandardStreamMetadataFormat = false;

    static final String nativeStreamMetadataFormatName = null;

    static final String nativeStreamMetadataFormatClassName = null;

    static final String[] extraStreamMetadataFormatNames = { null };

    static final String[] extraStreamMetadataFormatClassNames = { null };

    // ImageMetadataFormatNames and ImageMetadataFormatClassNames
    static final boolean supportsStandardImageMetadataFormat = false;

    static final String nativeImageMetadataFormatName = null;

    static final String nativeImageMetadataFormatClassName = null;

    static final String[] extraImageMetadataFormatNames = { null };

    static final String[] extraImageMetadataFormatClassNames = { null };

    public JP2KOpenJpegImageReaderSpi() {
        super(
                vendorName,
                version,
                formatNames,
                suffixes,
                MIMETypes,
                readerCN, // readerClassName
                new Class[] { File.class, byte[].class, ImageInputStream.class, URL.class, List.class },
                wSN, // writer Spi Names
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
		libraries = Arrays.asList("openjpegjni", "openjp2");
        _utilities.loadLibraries(libraries);
        format = "jpeg2000";
    }
    
	protected DecoderBase CreateDecoder()
	{
		
		return new OpenJPEGJavaDecoder();
	}


    /**
     * Returns an instance of the {@link JP2KOpenJpegImageReader}
     * 
     * @see javax.imageio.spi.ImageReaderSpi#createReaderInstance(java.lang.Object)
     */
    public ImageReader createReaderInstance(Object source) throws IOException {
        return new JP2KOpenJpegImageReader(this);
    }

    /**
     * @see javax.imageio.spi.IIOServiceProvider#getDescription(java.util.Locale)
     */
    public String getDescription(Locale locale) {
        return new StringBuffer("JP2K Image Reader, version ").append(version).toString();
    }

  
}
