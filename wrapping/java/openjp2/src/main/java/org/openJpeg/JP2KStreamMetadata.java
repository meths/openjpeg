package org.openJpeg;



import javax.imageio.metadata.IIOInvalidTreeException;
import javax.imageio.metadata.IIOMetadata;


import org.w3c.dom.Node;

public class JP2KStreamMetadata extends IIOMetadata {

	@Override
	public boolean isReadOnly() {
		// TODO Auto-generated method stub
		return false;
	}

	@Override
	public Node getAsTree(String formatName) {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public void mergeTree(String formatName, Node root)
			throws IIOInvalidTreeException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void reset() {
		// TODO Auto-generated method stub
		
	}

  

}
