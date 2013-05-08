package com.utils;

import java.io.*;

/**
 * Get basic configuration for this server
 * @author hoangtung
 *
 */
public class Config {
	private static String config = "config.txt";
	private static String driver = null;
	private static String dbUrl = null;
	private static String pass = null;
	private static String user = null;
	private static String pathFiles = null;
	private static String pathUploads = null;
	private static int portNum = -1;
	
	static {
		try {
			@SuppressWarnings("resource")
			BufferedReader br = new BufferedReader(new FileReader(config));
			driver = br.readLine();
			dbUrl = br.readLine();
			user = br.readLine();
			pass = br.readLine();
			portNum = Integer.valueOf(br.readLine());
			pathFiles = br.readLine();
			pathUploads = br.readLine();
		}
		catch (Exception exc) {
			exc.printStackTrace();
		}
	}
	
	public static String getDriverString() {
		return driver;
	}
	
	public static String getDBUrl() {
		return dbUrl;
	}
	
	public static String getPass() {
		return pass;
	}
	
	public static String getUser() {
		return user;
	}
	
	public static int getPortNum() {
		return portNum;
	}

	public static String getPathFiles() {
		return pathFiles;
	}

	public static String getPathUploads() {
		return pathUploads;
	}

	
	
	
}
