package org.openJpeg;


import javax.imageio.ImageReadParam;
import javax.imageio.ImageTypeSpecifier;


public class JP2KOpenJpegImageReadParam extends ImageReadParam {

    public Object clone() throws CloneNotSupportedException {
        final JP2KOpenJpegImageReadParam retVal = new JP2KOpenJpegImageReadParam();
        retVal.setController(this.getController());
        retVal.setDestination(getDestination());
        retVal.setDestinationBands(getDestinationBands());
        retVal.setDestinationOffset(getDestinationOffset());
        retVal.setDestinationType(getDestinationType());
        retVal.setSourceBands(getSourceBands());
        retVal.setSourceProgressivePasses(getSourceMinProgressivePass(),
                getSourceNumProgressivePasses());
        retVal.setSourceRegion(getSourceRegion());
        try {
            retVal.setSourceRenderSize(getSourceRenderSize());
        } catch (Throwable t) {
        }
        retVal.setSourceSubsampling(getSourceXSubsampling(),
                getSourceYSubsampling(), getSubsamplingXOffset(),
                getSubsamplingYOffset());
        return retVal;
    }


    protected void initialize(ImageReadParam param) {
        if (param.hasController()) 
            setController(param.getController());
        setSourceRegion(param.getSourceRegion());
        setSourceBands(param.getSourceBands());
        setDestinationBands(param.getDestinationBands());
        setDestination(param.getDestination());
        setDestinationOffset(param.getDestinationOffset());
        setSourceSubsampling(param.getSourceXSubsampling(), param
                .getSourceYSubsampling(), param.getSubsamplingXOffset(), param
                .getSubsamplingYOffset());
        final ImageTypeSpecifier type = param.getDestinationType();
        if (type != null)
        	setDestinationType(type);

    }
}
