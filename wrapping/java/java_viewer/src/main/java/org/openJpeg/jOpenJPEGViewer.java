/*
 * jOpenJPEGViewer.java - view JPEG2000 images
 *
 */

package org.openJpeg;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import java.util.Iterator;

import javax.imageio.ImageIO;
import javax.imageio.ImageReader;
import javax.imageio.spi.IIORegistry;
import javax.swing.JButton;
import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JMenu;
import javax.swing.JMenuBar;
import javax.swing.JMenuItem;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextField;

import org.openJpeg.JP2KOpenJpegImageReaderSpi;
import org.openJpeg.OpenJPEGJavaDecoder;

public class jOpenJPEGViewer extends JFrame {
	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	static jOpenJPEGViewer viewer;
	private StringBuilder read_idf;
	private StringBuilder jp2_file;
	private BufferedImage bufimg;
	private ImageCanvas canvas;
	private JScrollPane scroller;
	private JPanel header;
	private JPanel url;
	private OpenJPEGJavaDecoder decoder;
	private JTextField tile_in;
	private JTextField tile_out;
	private JTextField reduction_in;
	private JTextField reduction_out;
	private JTextField url_out;
	private JButton reload_file;
	private JTextField area_in;
	private int width = -1, height = -1;
	private int SW = -1, SH = -1;
	private int SHADOW_W = 4;
	/*---------------------------------------------*/
	private final int maxSW = 750, maxSH = 465;
	private final int minSW = 645, minSH = 185;
	/*---------------------------------------------*/
	private final int barSIZE = 18;
	private final int headerH = 35;
	private String dname = ".";
	private int max_tiles = 0;
	private int max_reduction = 0;
	private int cur_reduction = 0;
	private boolean reloaded = false;

	class ImageCanvas extends JPanel {

		/**
		 * 
		 */
		private static final long serialVersionUID = 1L;

		public void paintComponent(Graphics g) {
			super.paintComponent(g);

			Graphics2D g2 = (Graphics2D) g;

			if (bufimg != null)
				g2.drawImage(bufimg, 0, 0, width, height, this);
		}
	}/* class ImageCanvas */

	jOpenJPEGViewer() {
		super();

		setTitle("JPEG2000 jOpenJPEGViewer");
		getContentPane().setBackground(Color.LIGHT_GRAY);

		JMenuBar mBar = new JMenuBar();
		JMenu menuFile = new JMenu();
		JMenuItem menuFileOpen = new JMenuItem();
		JMenuItem menuFileExit = new JMenuItem();

		menuFile.setText("FILE");
		menuFileOpen.setText("Open");

		menuFileOpen.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				open_cb();
			}
		});

		menuFileExit.setText("Exit");

		menuFileExit.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				decoder.reset();
				System.exit(0);
			}
		});

		menuFile.add(menuFileOpen);
		menuFile.addSeparator();
		menuFile.add(menuFileExit);
		mBar.add(menuFile);

		setJMenuBar(mBar);

		header = new JPanel();
		header.setSize(minSW, headerH);

		JLabel label = new JLabel("Tile:");
		header.add(label);
		tile_in = new JTextField(3);

		tile_in.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				tile_in_cb();
			}
		});
		header.add(tile_in);

		label = new JLabel(" of ");
		header.add(label);
		tile_out = new JTextField(3);
		tile_out.setEditable(false);
		header.add(tile_out);

		label = new JLabel("Reduction:");
		header.add(label);
		reduction_in = new JTextField(3);

		reduction_in.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				reduction_in_cb();
			}
		});
		header.add(reduction_in);

		label = new JLabel("of:");
		header.add(label);
		reduction_out = new JTextField(3);
		reduction_out.setEditable(false);
		header.add(reduction_out);

		reload_file = new JButton("Reload");

		reload_file.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				reload_file_cb();
			}
		});
		header.add(reload_file);

		label = new JLabel("Area:");
		header.add(label);
		area_in = new JTextField("x0,y0,x1,y1", 15);

		area_in.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				area_in_cb();
			}
		});
		header.add(area_in);

		getContentPane().add(header, BorderLayout.NORTH);

		url = new JPanel();
		url.setSize(minSW, headerH);

		label = new JLabel("URL:");
		url.add(label);

		url_out = new JTextField(40);
		url_out.setEditable(false);
		url.add(url_out);

		getContentPane().add(url, BorderLayout.CENTER);

		canvas = new ImageCanvas();

		canvas.setBackground(Color.white);

		scroller = new JScrollPane(canvas);
		scroller.setPreferredSize(new Dimension(minSW, minSH));

		getContentPane().add(scroller, BorderLayout.SOUTH);

		setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

		decoder = new OpenJPEGJavaDecoder();

		read_idf = new StringBuilder();
		jp2_file = new StringBuilder();
		
	}/* jOpenJPEGViewer */

	protected void show_image() {
		String s, ws="", hs="", full, base;
		bufimg = null;

		test_new_jp2_file();
		full = this.read_idf.toString();
		
		bufimg = null;
		Iterator<ImageReader> readers = ImageIO.getImageReadersByFormatName("jpeg2000");
        if (!readers.hasNext()) {
            readers = ImageIO.getImageReadersByFormatName("jpeg2000");
        }
        ImageReader reader = readers.next();
		test_new_jp2_file();
		full = this.read_idf.toString();
		
		try {
			reader.setInput(new File(full));
			bufimg = reader.read(1);
			width = bufimg.getWidth();
			height = bufimg.getHeight();
			ws = String.valueOf(width);
			hs = String.valueOf(height);
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

		base = full;
		s = base + "(" + ws + "x" + hs + ")";

		url_out.setText(s);

		max_tiles = /*decoder.getMaxTiles()*/1;
		max_reduction = /*decoder.getMaxReduction()*/0;
		reduction_in.setText(String.valueOf(cur_reduction));
		tile_out.setText(String.valueOf(max_tiles - 1));
		reduction_out.setText(String.valueOf(max_reduction - 1));


		if (bufimg != null) {
			int has_hbar = 0, has_vbar = 0;

			if (width > maxSW - barSIZE)
				has_hbar = barSIZE;
			if (height > maxSH - barSIZE)
				has_vbar = barSIZE;

			SW = width + SHADOW_W;
			SH = height + SHADOW_W;

			if (width > maxSW - barSIZE)
				SW = maxSW;
			else if (width < minSW)
				SW = minSW;

			if (height > maxSH - barSIZE)
				SH = maxSH;

			SW += has_vbar;
			SH += has_hbar;

			header.setPreferredSize(new Dimension(SW, headerH));
			url.setPreferredSize(new Dimension(SW, headerH));
			scroller.setPreferredSize(new Dimension(SW, SH));
			canvas.setPreferredSize(new Dimension(width, height));
			canvas.revalidate();
			pack();
			scroller.getVerticalScrollBar().getModel().setValue(0);
			scroller.getHorizontalScrollBar().getModel().setValue(0);
		} /* if(bufimg != null) */

		canvas.repaint();

	}/* show_image() */

	protected void open_cb() {
		class OpjFilter extends javax.swing.filechooser.FileFilter {

			public String getDescription() {
				return "*.jp2;*.j2k;*.jpc;*.j2c;*.jpt";
			}

			public boolean accept(File f) {
				if (f == null)
					return false;

				if (f.isDirectory())
					return true;

				return f.getName().toLowerCase().endsWith(".jp2")
						|| f.getName().toLowerCase().endsWith(".j2k")
						|| f.getName().toLowerCase().endsWith(".jpc")
						|| f.getName().toLowerCase().endsWith(".j2c")
						|| f.getName().toLowerCase().endsWith(".jpt");
			} /* accept() */
		} /* class OpjFilter */

		JFileChooser openDialog = new JFileChooser();
		openDialog.setCurrentDirectory(new File(dname));
		openDialog.setFileFilter(new OpjFilter());
		openDialog.setAcceptAllFileFilterUsed(false);
		openDialog.setMultiSelectionEnabled(true);

		if (JFileChooser.APPROVE_OPTION == openDialog.showOpenDialog(this)) {
			this.read_idf.delete(0, this.read_idf.length());

			this.read_idf.append(openDialog.getSelectedFile().getPath());

			if (this.read_idf.length() != 0) {
				try {
					dname = openDialog.getCurrentDirectory().getCanonicalPath();
				} catch (Exception e) {
					e.printStackTrace();
				}

				show_image();

			} /* if(this.read_idf.length() != 0) */
		} /* if(JFileChooser */
	}/* protected void open_cb() */

	protected void tile_in_cb() {
		if (this.read_idf.length() != 0) {
			String s;
			int i;

			s = tile_in.getText().trim();

			if (s.length() == 0)
				i = -1;
			else
				i = Integer.parseInt(s);

			if (i < 0 || i >= max_tiles)
				return;

			decoder.setTileIn(i);

			decoder.setUserChangedTile(1);
			decoder.setUserChangedReduction(1);
			decoder.setUserChangedArea(0);

			show_image();

		} /* if(this.read_idf.length() != 0) */

	}

	protected void reduction_in_cb() {
		String s;
		int i;

		s = reduction_in.getText().trim();

		if (s.length() == 0)
			i = -1;
		else
			i = Integer.parseInt(s);

		if (i < 0)
			return;
		if (max_reduction > 0 && i >= max_reduction)
			return;

		cur_reduction = i;
		decoder.setUserChangedTile(1);
		decoder.setUserChangedReduction(1);
		decoder.setReductionIn(i);

		if (this.read_idf.length() != 0) {
			show_image();
		}
	}

	protected void test_new_jp2_file() {
		String j, r;
		boolean new_jp2_file;

		if (reloaded == true) {
			reloaded = false;
			/*
			 * Load the original file geometry:
			 */
			decoder.setUserChangedArea(0);
			decoder.setAreaIn(0, 0, 0, 0);

			decoder.setMaxTiles(0);
			decoder.setMaxReduction(0);

			tile_in.setText(" ");
			reduction_in.setText("0");

			return;
		}
		j = jp2_file.toString();
		r = read_idf.toString();

		if (jp2_file.length() > 0) {
			if (j.equals(r) == false)/* New file */
			{
				new_jp2_file = true;
			} else {
				new_jp2_file = false;
			}
		} else {
			new_jp2_file = true;
		}

		if (new_jp2_file == true) {
			jp2_file.delete(0, jp2_file.length());
			jp2_file.append(r);

			decoder.setUserChangedArea(0);
			decoder.setAreaIn(0, 0, 0, 0);

			decoder.setMaxTiles(0);
			decoder.setMaxReduction(0);

			tile_in.setText(" ");
			reduction_in.setText("0");
		}
	}/* test_new_jp2_file() */

	protected void reload_file_cb() {
		tile_in.setText(" ");
		reduction_in.setText("0");

		cur_reduction = 0;

		decoder.setUserChangedTile(0);
		decoder.setUserChangedReduction(0);
		decoder.setTileIn(-1);
		decoder.setReductionIn(0);

		reloaded = true;

		if (this.read_idf.length() != 0) {
			show_image();
		}
	}

	protected void area_in_cb() {
		String s;
		String[] r;
		int x0, y0, x1, y1;

		s = area_in.getText().trim();

		if (s.length() == 0) {
			area_in.setText("x0,y0,x1,y1");
			return;
		}
		r = s.split(",");

		if (r.length != 4) {
			System.out.println("AREA(" + s + ") unusable");
			return;
		}

		x0 = Integer.parseInt(r[0]);
		y0 = Integer.parseInt(r[1]);
		x1 = Integer.parseInt(r[2]);
		y1 = Integer.parseInt(r[3]);

		decoder.setAreaIn(x0, y0, x1, y1);
		// area_in.setText("x0,y0,x1,y1");/* szukw000 FIXME */

		decoder.setUserChangedArea(1);
		decoder.setUserChangedTile(0);
		decoder.setUserChangedReduction(0);

		tile_in.setText(" ");
		reduction_in.setText("0");

		if (this.read_idf.length() != 0) {
			show_image();
		}
	}

	public static void main(String args[]) {
		jOpenJPEGViewer viewer = new jOpenJPEGViewer();
		viewer.setLocation(100, 100);
		viewer.pack();
		viewer.setVisible(true);
	}
}
