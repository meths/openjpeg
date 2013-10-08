package org.openJpeg;


import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.imageio.spi.IIORegistry;
import javax.imageio.spi.ImageReaderSpi;
import javax.imageio.spi.ImageReaderWriterSpi;
import javax.imageio.spi.ImageWriterSpi;
import javax.imageio.spi.ServiceRegistry;



public class OpenJpegUtilities {

    public static final double DOUBLE_TOLERANCE = 1E-6;

    private static final Logger LOGGER = Logger
            .getLogger("org.openJpeg.imageio_openjpeg");

    /** is OpenJpeg available on this machine?. */
    private static boolean available;

    private static boolean init = false;

    public static final double BIT_TO_BYTE_FACTOR = 0.125;
    
/*----------------------------------------------------------------------*/

    private OpenJpegUtilities() {

    }

  
    public static List<ImageReaderWriterSpi> getJDKImageReaderWriterSPI(
            ServiceRegistry registry, String formatName, boolean isReader) {

        if (registry == null || !(registry instanceof IIORegistry))
            throw new IllegalArgumentException("Illegal registry provided");

        IIORegistry iioRegistry = (IIORegistry) registry;
        Class<? extends ImageReaderWriterSpi> spiClass;
        if (isReader)
            spiClass = ImageReaderSpi.class;
        else
            spiClass = ImageWriterSpi.class;

        final Iterator<? extends ImageReaderWriterSpi> iter = iioRegistry
                .getServiceProviders(spiClass, true); // useOrdering
        final ArrayList<ImageReaderWriterSpi> list = new ArrayList<ImageReaderWriterSpi>();
        while (iter.hasNext()) {
            final ImageReaderWriterSpi provider = (ImageReaderWriterSpi) iter
                    .next();

            // Get the formatNames supported by this Spi
            final String[] formatNames = provider.getFormatNames();
            for (int i = 0; i < formatNames.length; i++) {
                if (formatNames[i].equalsIgnoreCase(formatName)) {
                    // Must be a JDK provided ImageReader/ImageWriter
                    list.add(provider);
                    break;
                }
            }
        }
        return list;
    }


    /**
     * Returns <code>true</code> if the OpenJpeg native library has been loaded.
     * <code>false</code> otherwise.
     * 
     * @return <code>true</code> only if the OpenJpeg native library has been
     *         loaded.
     */
    public static boolean isOpenJpegAvailable(String openJPEGlibrary,String openJPEGJNI) {
        loadOpenJpeg(openJPEGlibrary, openJPEGJNI);
        return available;
    }

    /**
     * Forces loading of OpenJpeg libs.
     */
    public synchronized static void loadOpenJpeg(String openJPEGlibrary,String openJPEGJNI) {
        if (init == false)
            init = true;
        else
            return;
        try {
        	if (openJPEGlibrary != null)
				System.loadLibrary(openJPEGlibrary);
			if (openJPEGJNI != null)
				System.loadLibrary(openJPEGJNI);
            available = true;
        } catch (UnsatisfiedLinkError e) {
            if (LOGGER.isLoggable(Level.WARNING)){
            	 LOGGER.warning("Failed to load the OpenJpeg native libs. This is not a problem unless you need to use the OpenJpeg plugin: it won't be enabled. " + e.toString());
            }
            available = false;
        }
    }
    
  
}
