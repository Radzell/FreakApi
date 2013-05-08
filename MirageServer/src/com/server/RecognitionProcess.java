package com.server;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.util.Iterator;
import java.util.Vector;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.logging.Logger;

import com.entity.KeyPoint;
import com.entity.Mat;
import com.entity.TargetImage;
import com.utils.Config;
import com.utils.Util;

public class RecognitionProcess{

	Connection con;
	boolean isRunning = false;

	private static int NUM_OF_TASKS = 1;
	int cnt = 0;
	long begTest, endTest;
	ExecutorService es;
	Vector<Integer> resultIds;

	private static Logger logger = Logger.getLogger(RecognitionProcess.class.getName());


	public RecognitionProcess() {
		try {
			Class.forName(Config.getDriverString()).newInstance();
			con = DriverManager.getConnection(Config.getDBUrl(), Config.getUser(), Config.getPass());
		} catch (Exception e) {
			Util.writeLog(logger, e);
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}

	/**
	 * Reconnect to sql server if connection was broken
	 * 
	 * @return
	 */
	private boolean reconnect() {
		try {
			Class.forName(Config.getDriverString()).newInstance();
			con = DriverManager.getConnection(Config.getDBUrl(), Config.getUser(), Config.getPass());
		} catch (Exception e) {
			Util.writeLog(logger, e);
			// TODO Auto-generated catch block
			e.printStackTrace();
			return false;
		}

		return true;
	}
	
	public void start(){
		if(!isRunning){
			run();
		}
	}

	private void run() {
		while (true) {
			isRunning = true;
			Job job = Util.getJob();
			if (job != null) {
				startProcess(job);
			}else{
				break;
			}
		}
		isRunning = false;
	}

	public void startProcess(Job job) {
		int nrOfProcessors = Runtime.getRuntime().availableProcessors();
		es = Executors.newFixedThreadPool(nrOfProcessors);

		resultIds = new Vector<Integer>();

		int targetsPerCore = (int) Util.numTargets / nrOfProcessors;
		int extLastCore = 0;

		if ((targetsPerCore * nrOfProcessors) != Util.numTargets) {
			extLastCore = Util.numTargets - (targetsPerCore * nrOfProcessors);
		}

		NUM_OF_TASKS = nrOfProcessors;

		int beginThisCore = 0;
		int endThisCore = beginThisCore + targetsPerCore - 1;
		
		for (int i = 0; i < NUM_OF_TASKS; i++) {
			if (i == (NUM_OF_TASKS - 1)) {
				extLastCore = endThisCore - Util.numTargets;
				endThisCore -= extLastCore + 1;
			}
			CallBackTask task = new CallBackTask(i, beginThisCore, endThisCore, job);
			task.setCaller(this);
			es.submit(task);

			beginThisCore = endThisCore + 1;
			endThisCore = beginThisCore + targetsPerCore;
		}
	}

	public void callBack(Vector<Integer> result, Job job) {
		// if (sizeResult > 0) {
		// es.shutdown();
		// }
		if (result.size() > 0) {
			for (int i = 0; i < result.size(); i++) {
				resultIds.add(result.get(i));
			}
		}

		if (++cnt == NUM_OF_TASKS) {
			ResponseHandler responseHandler = new ResponseHandler(job.getIp(),job.getHostname());
			try {
				Vector<TargetImage> b = getTargetImages(resultIds);
				// Vector<TargetImage> b =
				// getTargetImages(Matcher.match("/home/diego/MirageFiles/uploads/"
				// + imageName));
				responseHandler.responseTargetImages(b);
			} catch (Exception exc) {
				Util.writeLog(logger, exc);
				responseHandler.responseError("Cannot find book");
				exc.printStackTrace();
				reconnect();
			}
			es.shutdown();
			cnt = 0;
			

		}
	}

	private Vector<TargetImage> getTargetImages(Vector<Integer> ids) throws SQLException {
		Vector<TargetImage> result = new Vector<TargetImage>();

		result.setSize(ids.size());
		int id, rc, p;
		String tit, au, in, ta, img;
		float ra;

		byte[] kypbt;
		byte[] dessbt;
		Vector<KeyPoint> kyp;

		Mat dess;

		String kypst;
		String dessst;

		// create a sql statement for selecting books with the listed ids
		String sql = "select id, _name, _author, _description, _rating, _rateCount, _image,_keypoint,_descriptor from targetimage where id in (";
		int idSize = ids.size();
		Iterator<Integer> it = ids.iterator();
		for (int i = 0; i < idSize; ++i) {
			sql += it.next();
			if (i < idSize - 1) {
				sql += ", ";
			}
		}
		sql += ")";
		// System.out.println("SQL " + sql);
		if (ids.size() > 0) {
			ResultSet rs = con.createStatement().executeQuery(sql);
			// get the result and reorder it
			while (rs.next()) {
				// System.out.println("Add one book");
				id = rs.getInt(1);
				tit = rs.getNString(2);
				au = rs.getNString(3);
				in = rs.getNString(4);
				ra = rs.getFloat(5);
				rc = rs.getInt(6);
				img = rs.getString(7);
				kypbt = (byte[]) rs.getObject(8);
				kyp = (Vector<KeyPoint>) Util.objectFromByteArray(kypbt);
				dessbt = (byte[]) rs.getObject(9);
				dess = (Mat) Util.objectFromByteArray(dessbt);

				int idx = ids.indexOf(id);
				// System.out.println("Idx " + idx);

				result.set(idx, new TargetImage(id, tit, au, in, ra, rc, img, kypbt, dessbt));
			}
		}

		return result;
	}

}