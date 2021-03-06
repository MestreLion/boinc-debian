/*******************************************************************************
 * This file is part of BOINC.
 * http://boinc.berkeley.edu
 * Copyright (C) 2012 University of California
 * 
 * BOINC is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 * 
 * BOINC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with BOINC.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/
package edu.berkeley.boinc.client;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.HashMap;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Binder;
import android.os.IBinder;
import android.util.Log;
import edu.berkeley.boinc.AppPreferences;
import edu.berkeley.boinc.R;
import edu.berkeley.boinc.rpc.AccountIn;
import edu.berkeley.boinc.rpc.AccountOut;
import edu.berkeley.boinc.rpc.CcStatus;
import edu.berkeley.boinc.rpc.GlobalPreferences;
import edu.berkeley.boinc.rpc.Message;
import edu.berkeley.boinc.rpc.Project;
import edu.berkeley.boinc.rpc.ProjectAttachReply;
import edu.berkeley.boinc.rpc.ProjectInfo;
import edu.berkeley.boinc.rpc.ProjectConfig;
import edu.berkeley.boinc.rpc.Result;
import edu.berkeley.boinc.rpc.RpcClient;
import edu.berkeley.boinc.rpc.Transfer;

public class Monitor extends Service {
	
	private static final String TAG = "BOINC Monitor Service";
	
	private static ClientStatus clientStatus; //holds the status of the client as determined by the Monitor
	private static AppPreferences appPrefs; //hold the status of the app, controlled by AppPreferences
	
	public static Boolean monitorActive = false;
	
	private String clientName; 
	private String clientCLI; 
	private String clientCABundle; 
	private String authFileName; 
	private String allProjectsList; 
	private String globalOverridePreferences;
	private String clientPath; 
	
	private Boolean started = false;
	private Thread monitorThread = null;
	private Boolean monitorRunning = true;
	
	//private Process clientProcess;
	private RpcClient rpc = new RpcClient();

	private final Integer maxDuration = 3000; //maximum polling duration
	
	private Integer retryRate;
	private Integer retryAttempts;


	// installs client and required files, executes client and reads initial preferences
	// used by ClientMonitorAsync if no connection is available
	// includes network communication => don't call from UI thread!
	private Boolean clientSetup() {
		getClientStatus().setSetupStatus(ClientStatus.SETUP_STATUS_LAUNCHING,true);
		String clientProcessName = clientPath + clientName;

		String md5AssetClient = ComputeMD5Asset(clientName);
		Log.d(TAG, "Hash of client (Asset): '" + md5AssetClient + "'");

		String md5InstalledClient = ComputeMD5File(clientProcessName);
		Log.d(TAG, "Hash of client (File): '" + md5InstalledClient + "'");

		// If client hashes do not match, we need to install the one that is a part
		// of the package. Shutdown the currently running client if needed.
		//
		if (md5InstalledClient.compareToIgnoreCase(md5AssetClient) != 0) {

			// Determine if BOINC is already running.
			//
			quitProcessOsLevel(clientProcessName);

			// Install BOINC client software
			//
	        if(!installClient()) {
	        	Log.d(TAG, "BOINC client installation failed!");
	        	return false;
	        }
		}
		
		// Start the BOINC client if we need to.
		//
		Integer clientPid = getPidForProcessName(clientProcessName);
		if(clientPid == null) {
        	Log.d(TAG, "Starting the BOINC client");
			if (!runClient()) {
	        	Log.d(TAG, "BOINC client failed to start");
				return false;
			}
		}

		
		// Try to connect to executed Client in loop
		//
		Boolean connected = false;
		Integer counter = 0;
		while(!connected && (counter < retryAttempts)) {
			Log.d(TAG, "Attempting BOINC client connection...");
			connected = connectClient();
			counter++;

			try {
				Thread.sleep(retryRate);
			} catch (Exception e) {}
		}
		
		if(connected) {
			// if connection is set up, initial read of preferences
			GlobalPreferences clientPrefs = rpc.getGlobalPrefsWorkingStruct();
			Monitor.getClientStatus().setPrefs(clientPrefs);
		}
		
		if(connected) {
			Log.d(TAG, "setup completed successfully"); 
			getClientStatus().setSetupStatus(ClientStatus.SETUP_STATUS_AVAILABLE,false);
		} else {
			Log.d(TAG, "onPostExecute - setup experienced an error"); 
			getClientStatus().setSetupStatus(ClientStatus.SETUP_STATUS_ERROR,true);
		}
		
		return connected;
	}
	
    // Executes the BOINC client using the Java Runtime exec method.
	//
    private Boolean runClient() {
    	Boolean success = false;
    	try { 
    		String[] cmd = new String[2];
    		
    		cmd[0] = clientPath + clientName;
    		cmd[1] = "--daemon";
    		
        	Runtime.getRuntime().exec(cmd, null, new File(clientPath));
        	success = true;
    	} catch (IOException e) {
    		Log.d(TAG, "Starting BOINC client failed with exception: " + e.getMessage());
    		Log.e(TAG, "IOException", e);
    	}
    	return success;
    }

	private Boolean connectClient() {
		Boolean success = false;
		
        success = connect();
        if(!success) {
        	Log.d(TAG, "connection failed!");
        	return success;
        }
        
        //authorize
        success = authorize();
        if(!success) {
        	Log.d(TAG, "authorization failed!");
        }
        return success;
	}
	
	// Copies the binaries of BOINC client from assets directory into 
	// storage space of this application
	//
    private Boolean installClient(){

		installFile(clientName, true, true);
		installFile(clientCLI, true, true);
		installFile(clientCABundle, true, false);
		installFile(allProjectsList, true, false);
		installFile(globalOverridePreferences, false, false);
    	
    	return true; 
    }
    
	private Boolean installFile(String file, Boolean override, Boolean executable) {
    	Boolean success = false;
    	byte[] b = new byte [1024];
		int count; 
		
		try {
			Log.d(TAG, "installing: " + file);
			
    		File target = new File(clientPath + file);
    		
    		// Check path and create it
    		File installDir = new File(clientPath);
    		if(!installDir.exists()) {
    			installDir.mkdir();
    			installDir.setWritable(true); 
    		}
    		
    		// Delete old target
    		if(override && target.exists()) {
    			target.delete();
    		}
    		
    		// Copy file from the asset manager to clientPath
    		InputStream asset = getApplicationContext().getAssets().open(file); 
    		OutputStream targetData = new FileOutputStream(target); 
    		while((count = asset.read(b)) != -1){ 
    			targetData.write(b, 0, count);
    		}
    		asset.close(); 
    		targetData.flush(); 
    		targetData.close();

    		success = true; //copy succeeded without exception
    		
    		// Set executable, if requested
    		Boolean isExecutable = false;
    		if(executable) {
    			target.setExecutable(executable);
    			isExecutable = target.canExecute();
    			success = isExecutable; // return false, if not executable
    		}

    		Log.d(TAG, "install of " + file + " successfull. executable: " + executable + "/" + isExecutable);
    		
    	} catch (IOException e) {  
    		Log.d(TAG, "IOException: " + e.getMessage());
    		Log.e(TAG, "IOException", e);
    		
    		Log.d(TAG, "install of " + file + " failed.");
    	}
		
		return success;
	}

    // Connects to running BOINC client.
    //
    private Boolean connect() {
    	return rpc.open("127.0.0.1", 31416);
    }
    
    // Authorizes this application as valid RPC Manager by reading auth token from file 
    // and making RPC call.
    //
    private Boolean authorize() {
    	String authKey = readAuthToken();
		
		//trigger client rpc
		return rpc.authorize(authKey); 
    }
	
    // Compute MD5 of the requested asset
    //
    private String ComputeMD5Asset(String file) {
    	byte[] b = new byte [1024];
		int count; 
		
		try {
			MessageDigest md5 = MessageDigest.getInstance("MD5");

			InputStream asset = getApplicationContext().getAssets().open(file); 
    		while((count = asset.read(b)) != -1){ 
    			md5.update(b, 0, count);
    		}
    		asset.close();
    		
			byte[] md5hash = md5.digest();
			StringBuilder sb = new StringBuilder();
			for (int i = 0; i < md5hash.length; ++i) {
				sb.append(String.format("%02x", md5hash[i]));
			}
    		
    		return sb.toString();
    	} catch (IOException e) {  
    		Log.d(TAG, "IOException: " + e.getMessage());
    		Log.e(TAG, "IOException", e);
    	} catch (NoSuchAlgorithmException e) {
    		Log.d(TAG, "NoSuchAlgorithmException: " + e.getMessage());
    		Log.e(TAG, "NoSuchAlgorithmException", e);
		}
		
		return "";
    }

    // Compute MD5 of the requested file
    //
    private String ComputeMD5File(String file) {
    	byte[] b = new byte [1024];
		int count; 
		
		try {
			MessageDigest md5 = MessageDigest.getInstance("MD5");

    		File target = new File(file);
    		InputStream asset = new FileInputStream(target); 
    		while((count = asset.read(b)) != -1){ 
    			md5.update(b, 0, count);
    		}
    		asset.close();

			byte[] md5hash = md5.digest();
			StringBuilder sb = new StringBuilder();
			for (int i = 0; i < md5hash.length; ++i) {
				sb.append(String.format("%02x", md5hash[i]));
			}
    		
    		return sb.toString();
    	} catch (IOException e) {  
    		Log.d(TAG, "IOException: " + e.getMessage());
    		Log.e(TAG, "IOException", e);
    	} catch (NoSuchAlgorithmException e) {
    		Log.d(TAG, "NoSuchAlgorithmException: " + e.getMessage());
    		Log.e(TAG, "NoSuchAlgorithmException", e);
		}
		
		return "";
    }
    
	// Get PID for process name using native 'ps' console command
    //
    private Integer getPidForProcessName(String processName) {
    	int count;
    	char[] buf = new char[1024];
    	StringBuffer sb = new StringBuffer();
    	
    	//run ps and read output
    	try {
	    	Process p = Runtime.getRuntime().exec("ps");
	    	p.waitFor();
	    	InputStreamReader isr = new InputStreamReader(p.getInputStream());
	    	while((count = isr.read(buf)) != -1)
	    	{
	    	    sb.append(buf, 0, count);
	    	}
    	} catch (Exception e) {
    		Log.d(TAG, "Exception: " + e.getMessage());
    		Log.e(TAG, "Exception", e);
    	}
    	
    	//parse output into hashmap
    	HashMap<String,Integer> pMap = new HashMap<String, Integer>();
    	String [] processLinesAr = sb.toString().split("\n");
    	for(String line : processLinesAr)
    	{
    		Integer pid;
    		String packageName;
    	    String [] comps = line.split("[\\s]+");
    	    if(comps.length != 9) {continue;}     
    	    pid = Integer.parseInt(comps[1]);
    	    packageName = comps[8];
    	    pMap.put(packageName, pid);
    	    //Log.d(TAG,"added: " + packageName + pid); 
    	}
    	
    	// Find required pid
    	return pMap.get(processName);
    }
    
    // Exit a process with OS signals SIGQUIT and SIGKILL
    //
    private void quitProcessOsLevel(String processName) {
    	Integer clientPid = getPidForProcessName(processName);
    	
    	// client PID could not be read, client already ended / not yet started?
    	if (clientPid == null) {
    		Log.d(TAG, "quitProcessOsLevel could not find PID, already ended or not yet started?");
    		return;
    	}
    	
    	Log.d(TAG, "quitProcessOsLevel for " + processName + ", pid: " + clientPid);
    	
    	// Do not just kill the client on the first attempt.  That leaves dangling 
		// science applications running which causes repeated spawning of applications.
		// Neither the UI or client are happy and each are trying to recover from the
		// situation.  Instead send SIGQUIT and give the client time to clean up.
		//
    	android.os.Process.sendSignal(clientPid, android.os.Process.SIGNAL_QUIT);
    	
    	// Wait for up to 30 seconds for the client to shutdown gracefully
    	Integer loopCounter = 0;
    	while((loopCounter < 6) && (getPidForProcessName(processName) != null)) {
    		loopCounter++;
			try {
				Thread.sleep(5000);
			} catch (Exception e) {}
    	}
    	
    	// Process is still alive, sind SIGKILL
    	clientPid = getPidForProcessName(processName);
    	if(clientPid != null) {
    		Log.d(TAG, "SIGQUIT failed. SIGKILL pid: " + clientPid);
    		android.os.Process.killProcess(clientPid);
    	}
    	
    	clientPid = getPidForProcessName(processName);
    	if(clientPid != null) {
    		Log.d(TAG, "SIGKILL failed. still living pid: " + clientPid);
    	}
    }
	
	public static ClientStatus getClientStatus() { //singleton pattern
		if (clientStatus == null) {
			Log.d(TAG,"WARNING: clientStatus not yet initialized");
		}
		return clientStatus;
	}
	
	public static AppPreferences getAppPrefs() { //singleton pattern
		if (appPrefs == null) {
			appPrefs = new AppPreferences();
		}
		return appPrefs;
	}

	
	/*
	 * returns this class, allows clients to access this service's functions and attributes.
	 */
	public class LocalBinder extends Binder {
        public Monitor getService() {
            return Monitor.this;
        }
    }
    private final IBinder mBinder = new LocalBinder();

    /*
     * gets called every-time an activity binds to this service, but not the initial start (onCreate and onStartCommand are called there)
     */
    @Override
    public IBinder onBind(Intent intent) {
    	//Log.d(TAG,"onBind");
        return mBinder;
    }
	
    /*
     * onCreate is life-cycle method of service. regardless of bound or started service, this method gets called once upon first creation.
     */
	@Override
    public void onCreate() {
		Log.d(TAG,"onCreate()");
		
		// populate attributes with XML resource values
		clientPath = getString(R.string.client_path); 
		clientName = getString(R.string.client_name); 
		clientCLI = getString(R.string.client_cli); 
		clientCABundle = getString(R.string.client_cabundle); 
		authFileName = getString(R.string.auth_file_name); 
		allProjectsList = getString(R.string.all_projects_list); 
		globalOverridePreferences = getString(R.string.global_prefs_override);

		retryRate = getResources().getInteger(R.integer.monitor_setup_connection_retry_rate_ms);
		retryAttempts = getResources().getInteger(R.integer.monitor_setup_connection_retry_attempts);
		
		// initialize singleton helper classes and provide application context
		clientStatus = new ClientStatus(this);
		getAppPrefs().readPrefs(this);
		
		if(!started) {
			started = true;
	        (new ClientMonitorAsync()).execute(new Integer[0]); //start monitor in new thread
	        //Log.d(TAG, "asynchronous monitor started!");
		}
		else {
			Log.d(TAG, "asynchronous monitor NOT started!");
		}

        //Toast.makeText(this, "BOINC Monitor Service Starting", Toast.LENGTH_SHORT).show();
	}
	
    @Override
    public void onDestroy() {
    	Log.d(TAG,"onDestroy()");
    	
        // Cancel the persistent notification.
    	//
    	((NotificationManager)getSystemService(Service.NOTIFICATION_SERVICE)).cancel(getResources().getInteger(R.integer.autostart_notification_id));
        
    	// Abort the ClientMonitorAsync thread
    	//
    	monitorRunning = false;
		monitorThread.interrupt();
		
		clientStatus.setWakeLock(false); // release wakeLock, if held.
    	
    	// Quit client here is not appropriate?!
		// Keep Client running until explecitely killed, independently from UI
		//quitClient();
        
        //android.widget.Toast.makeText(this, "BOINC Monitor Service Stopped", android.widget.Toast.LENGTH_SHORT).show();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {	
    	//this gets called after startService(intent) (either by BootReceiver or AndroidBOINCActivity, depending on the user's autostart configuration)
    	Log.d(TAG, "onStartCommand()");
		/*
		 * START_NOT_STICKY is now used and replaced START_STICKY in previous implementations.
		 * Lifecycle events - e.g. killing apps by calling their "onDestroy" methods, or killing an app in the task manager - does not effect the non-Dalvik code like the native BOINC Client.
		 * Therefore, it is not necessary for the service to get reactivated. When the user navigates back to the app (BOINC Manager), the service gets re-started from scratch.
		 * Con: After user navigates back, it takes some time until current Client status is present.
		 * Pro: Saves RAM/CPU.
		 * 
		 * For detailed service documentation see
		 * http://android-developers.blogspot.com.au/2010/02/service-api-changes-starting-with.html
		 */
		return START_NOT_STICKY;
    }

    //sends broadcast about login (or register) result for login activity
    /*
	private void sendLoginResultBroadcast(Integer type, Integer result, String message) {
        Intent loginResults = new Intent();
        loginResults.setAction("edu.berkeley.boinc.loginresults");
        loginResults.putExtra("type", type);
        loginResults.putExtra("result", result);
        loginResults.putExtra("message", message);
        getApplicationContext().sendBroadcast(loginResults,null);
	}*/
	
    public void restartMonitor() {
    	if(Monitor.monitorActive) { //monitor is already active, launch cancelled
    		Log.d(TAG, "monitor active - restart cancelled");
    	}
    	else {
        	Log.d(TAG,"restart monitor");
        	(new ClientMonitorAsync()).execute(new Integer[0]);
    	}
    }
    
    // force ClientMonitorAsync to start with loop.
    // This will read client status using RPCs and fire event eventually.
    public void forceRefresh() {
    	Log.d(TAG,"forceRefresh()");
    	if(monitorThread != null) {
    		monitorThread.interrupt();
    	}
    }
    
    // exits both, UI and BOINC client. 
    // BLOCKING! call from AsyncTask!
    public void quitClient() {
    	monitorRunning = false; // stops ClientMonitorAsync loop
    	monitorThread.interrupt(); // wakening ClientMonitorAsync from sleep
    	// ClientMonitorAsync is not using RPC anymore
    	
    	// set client status to SETUP_STATUS_CLOSING to adapt layout accordingly
		getClientStatus().setSetupStatus(ClientStatus.SETUP_STATUS_CLOSING,true);
    	
    	// try graceful shutdown via RPC
    	rpc.quit();
    	
    	// there might be still other AsyncTasks executing RPCs
    	// close sockets in a synchronized way
    	rpc.close();
    	
    	// there are now more RPCs going on
    	quitProcessOsLevel(clientPath + clientName);
    	
    	// set client status to SETUP_STATUS_CLOSED to adapt layout accordingly
		getClientStatus().setSetupStatus(ClientStatus.SETUP_STATUS_CLOSED,true);
    }
       
	public void setRunMode(Integer mode) {
		//execute in different thread, in order to avoid network communication in main thread and therefore ANR errors
		(new WriteClientRunModeAsync()).execute(mode);
	}
	
	// writes the given GlobalPreferences via RPC to the client
	// after writing, the active preferences are read back and
	// written to ClientStatus.
	public Boolean setGlobalPreferences(GlobalPreferences prefs) {

		Boolean retval1 = rpc.setGlobalPrefsOverrideStruct(prefs); //set new override settings
		Boolean retval2 = rpc.readGlobalPrefsOverride(); //trigger reload of override settings
		if(!retval1 || !retval2) {
			return false;
		}
		GlobalPreferences workingPrefs = rpc.getGlobalPrefsWorkingStruct();
		if(workingPrefs != null){
			Monitor.getClientStatus().setPrefs(workingPrefs);
			return true;
		}
		return false;
	}
	

	public String readAuthToken() {
		File authFile = new File(clientPath+authFileName);
    	StringBuffer fileData = new StringBuffer(100);
    	char[] buf = new char[1024];
    	int read = 0;
    	try{
    		BufferedReader br = new BufferedReader(new FileReader(authFile));
    		while((read=br.read(buf)) != -1){
    	    	String readData = String.valueOf(buf, 0, read);
    	    	fileData.append(readData);
    	    	buf = new char[1024];
    	    }
    		br.close();
    	}
    	catch (FileNotFoundException fnfe) {
    		Log.e(TAG, "auth file not found",fnfe);
    	}
    	catch (IOException ioe) {
    		Log.e(TAG, "ioexception",ioe);
    	}

		String authKey = fileData.toString();
		Log.d(TAG, "authKey: " + authKey);
		return authKey;
	}
	
	public ArrayList<ProjectInfo> getAndroidProjectsList() {
		ArrayList<ProjectInfo> allProjects = rpc.getAllProjectsList();
		ArrayList<ProjectInfo> androidProjects = new ArrayList<ProjectInfo>();
		
		//filter projects that do not support Android
		for (ProjectInfo project: allProjects) {
			if(project.platforms.contains(getString(R.string.boinc_platform_name))) {
				Log.d(TAG, project.name + " supports " + getString(R.string.boinc_platform_name));
				androidProjects.add(project);
			} 
		}
		return androidProjects;
	}
	
	public ProjectConfig getProjectConfig(String url) {
		ProjectConfig config = null;
		
    	Boolean success = rpc.getProjectConfig(url); //asynchronous call
    	if(success) { //only continue if attach command did not fail
    		// verify success of getProjectConfig with poll function
    		Integer counter = 0;
    		Integer sleepDuration = 500; //in milliseconds
    		Integer maxLoops = maxDuration / sleepDuration;
    		Boolean loop = true;
    		while(loop && (counter < maxLoops)) {
    			loop = false;
    			try {
    				Thread.sleep(sleepDuration);
    			} catch (Exception e) {}
    			counter ++;
    			config = rpc.getProjectConfigPoll();
    			if(config==null) {
    				return null;
    			}
    			if (config.error_num == -204) {
    				loop = true; //no result yet, keep looping
    			} else {
    				//final result ready
    				if(config.error_num == 0) { 
        				Log.d(TAG, "ProjectConfig retrieved: " + config.name);
    				} else {
    					Log.d(TAG, "final result with error_num: " + config.error_num);
    				}
    			}
    		}
    	}
		return config;
	}
	
	public Boolean attachProject(String url, String name, String authenticator) {
    	Boolean success = false;
    	success = rpc.projectAttach(url, authenticator, name); //asynchronous call to attach project
    	if(success) { //only continue if attach command did not fail
    		// verify success of projectAttach with poll function
    		success = false;
    		Integer counter = 0;
    		Integer sleepDuration = 500; //in milliseconds
    		Integer maxLoops = maxDuration / sleepDuration;
    		while(!success && (counter < maxLoops)) {
    			try {
    				Thread.sleep(sleepDuration);
    			} catch (Exception e) {}
    			counter ++;
    			ProjectAttachReply reply = rpc.projectAttachPoll();
    			Integer result = reply.error_num;
    			if(result == 0) {
    				success = true;
    			}
    		}
    	}
    	return success;
    }
	
	public Boolean checkProjectAttached(String url) {
		Boolean match = false;
		ArrayList<Project> attachedProjects = rpc.getProjectStatus();
		for (Project project: attachedProjects) {
			Log.d(TAG, project.master_url + " vs " + url);
			if(project.master_url.equals(url)) {
				match = true;
				continue;
			}
		}
		return match;
	}
	
	public AccountOut lookupCredentials(String url, String id, String pwd) {
    	Integer retval = -1;
    	AccountOut auth = null;
    	AccountIn credentials = new AccountIn();
    	credentials.email_addr = id;
    	credentials.passwd = pwd;
    	credentials.url = url;
    	Boolean success = rpc.lookupAccount(credentials); //asynch
    	if(success) { //only continue if lookupAccount command did not fail
    		//get authentication token from lookupAccountPoll
    		Integer counter = 0;
    		Integer sleepDuration = 500; //in milliseconds
    		Integer maxLoops = maxDuration / sleepDuration;
    		Boolean loop = true;
    		while(loop && (counter < maxLoops)) {
    			loop = false;
    			try {
    				Thread.sleep(sleepDuration);
    			} catch (Exception e) {}
    			counter ++;
    			auth = rpc.lookupAccountPoll();
    			if(auth==null) {
    				return null;
    			}
    			if (auth.error_num == -204) {
    				loop = true; //no result yet, keep looping
    			}
    			else {
    				//final result ready
    				retval = auth.error_num;
    				if(auth.error_num == 0) { 
        				Log.d(TAG, "credentials verification result, retrieved authenticator: " + auth.authenticator);
    				}
    			}
    		}
    	}
    	Log.d(TAG, "lookupCredentials returns " + retval);
    	return auth;
    }
	
	public Boolean detachProject(String url){
		return rpc.projectOp(RpcClient.PROJECT_DETACH, url);
	}
	
	public void detachProjectAsync(String url){
		Log.d(TAG, "detachProjectAsync");
		String[] param = new String[1];
		param[0] = url;
		(new ProjectDetachAsync()).execute(param);
	}
    
	public Boolean abortTransfer(String url, String name){
		return rpc.transferOp(RpcClient.TRANSFER_ABORT, url, name);
	}
	
	public void abortTransferAsync(String url, String name){
		Log.d(TAG, "abortTransferAsync");
		String[] param = new String[2];
		param[0] = url;
		param[1] = name;
		(new TransferAbortAsync()).execute(param);
	}
    
	public Boolean updateProject(String url){
		return rpc.projectOp(RpcClient.PROJECT_UPDATE, url);
	}
	
	public void updateProjectAsync(String url){
		Log.d(TAG, "updateProjectAsync");
		String[] param = new String[1];
		param[0] = url;
		(new ProjectUpdateAsync()).execute(param);
	}
    
	public Boolean retryTransfer(String url, String name){
		return rpc.transferOp(RpcClient.TRANSFER_RETRY, url, name);
	}
	
	public void retryTransferAsync(String url, String name){
		Log.d(TAG, "retryTransferAsync");
		String[] param = new String[2];
		param[0] = url;
		param[1] = name;
		(new TransferRetryAsync()).execute(param);
	}
	
	public AccountOut createAccount(String url, String email, String userName, String pwd, String teamName) {
		AccountIn information = new AccountIn();
		information.url = url;
		information.email_addr = email;
		information.user_name = userName;
		information.passwd = pwd;
		information.team_name = teamName;
		
		AccountOut auth = null;
		
    	Boolean success = rpc.createAccount(information); //asynchronous call to attach project
    	if(success) { //only continue if attach command did not fail
    		// verify success of projectAttach with poll function
    		Integer counter = 0;
    		Integer sleepDuration = 500; //in milliseconds
    		Integer maxLoops = maxDuration / sleepDuration;
    		Boolean loop = true;
    		while(loop && (counter < maxLoops)) {
    			loop = false;
    			try {
    				Thread.sleep(sleepDuration);
    			} catch (Exception e) {}
    			counter ++;
    			auth = rpc.createAccountPoll();
    			if(auth==null) {
    				return null;
    			}
    			if (auth.error_num == -204) {
    				loop = true; //no result yet, keep looping
    			}
    			else {
    				//final result ready
    				if(auth.error_num == 0) { 
        				Log.d(TAG, "account creation result, retrieved authenticator: " + auth.authenticator);
    				}
    			}
    		}
    	}
    	return auth;
	}
	
	private final class ClientMonitorAsync extends AsyncTask<Integer, String, Boolean> {

		private final String TAG = "BOINC ClientMonitorAsync";
		private final Boolean showRpcCommands = false;
		
		// Frequency of which the monitor updates client status via RPC, to often can cause reduced performance!
		private Integer refreshFrequency = getResources().getInteger(R.integer.monitor_refresh_rate_ms);
		
		@Override
		protected Boolean doInBackground(Integer... params) {
			// Save current thread, to interrupt sleep from outside...
			monitorThread = Thread.currentThread();
			Boolean sleep = true;
			while(monitorRunning) {
				publishProgress("doInBackground() monitor loop...");
				
				if(!rpc.connectionAlive()) { //check whether connection is still alive
					// If connection is not working, either client has not been set up yet or client crashed.
					clientSetup();
					sleep = false;
				} else {
					if(showRpcCommands) Log.d(TAG, "getCcStatus");
					CcStatus status = rpc.getCcStatus();
					/*
					if(showRpcCommands) Log.d(TAG, "getState"); 
					CcState state = rpc.getState();
					*/
					if(showRpcCommands) Log.d(TAG, "getResults");
					ArrayList<Result>  results = rpc.getResults();
					if(showRpcCommands) Log.d(TAG, "getProjects");
					ArrayList<Project>  projects = rpc.getProjectStatus();
					if(showRpcCommands) Log.d(TAG, "getTransers");
					ArrayList<Transfer>  transfers = rpc.getFileTransfers();
					ArrayList<Message> msgs = new ArrayList<Message>();
					// retrieve messages only, if tabs are actually enabled. very resource intense with logging on emulator!
					if(getResources().getBoolean(R.bool.tab_eventlog)) { 
						Integer count = rpc.getMessageCount();
						msgs = rpc.getMessages(count - 250); //get the most recent 250 messages
						if(showRpcCommands) Log.d(TAG, "getMessages, count: " + count);
					}
					
					if( (status != null) && (results != null) && (projects != null) && (transfers != null)) {
						Monitor.getClientStatus().setClientStatus(status, results, projects, transfers, msgs);
					} else {
						Log.d(TAG, "client status connection problem");
					}
					
			        Intent clientStatus = new Intent();
			        clientStatus.setAction("edu.berkeley.boinc.clientstatus");
			        getApplicationContext().sendBroadcast(clientStatus);
			        
			        sleep = true;
				}
				
				if(sleep) {
		    		try {
		    			Thread.sleep(refreshFrequency);
		    		} catch(InterruptedException e) {Log.d(TAG,"sleep interrupted");}
				}
			}

			return true;
		}

		@Override
		protected void onProgressUpdate(String... arg0) {
			Log.d(TAG, "onProgressUpdate() " + arg0[0]);
		}
		
		@Override
		protected void onPostExecute(Boolean success) {
			Log.d(TAG, "onPostExecute() monitor exit"); 
			Monitor.monitorActive = false;
		}
	}
	
	private final class ProjectDetachAsync extends AsyncTask<String,String,Boolean> {

		private final String TAG = "ProjectDetachAsync";
		
		private String url;
		
		@Override
		protected Boolean doInBackground(String... params) {
			this.url = params[0];
			Log.d(TAG+"-doInBackground","ProjectDetachAsync url: " + url);
			
			Boolean detach = rpc.projectOp(RpcClient.PROJECT_DETACH, url);
			if(detach) {
				Log.d(TAG, "successful.");
			}
			return detach;
		}
		
		@Override
		protected void onPostExecute(Boolean success) {
			forceRefresh();
		}
	}

	private final class ProjectUpdateAsync extends AsyncTask<String,String,Boolean> {

		private final String TAG = "ProjectUpdateAsync";
		
		private String url;
		
		@Override
		protected Boolean doInBackground(String... params) {
			this.url = params[0];
			Log.d(TAG, "doInBackground() - ProjectUpdateAsync url: " + url);
			
			Boolean update = rpc.projectOp(RpcClient.PROJECT_UPDATE, url);
			if(update) {
				Log.d(TAG, "successful.");
			}
			return update;
		}
		
		@Override
		protected void onPostExecute(Boolean success) {
			forceRefresh();
		}
	}

	private final class TransferAbortAsync extends AsyncTask<String,String,Boolean> {

		private final String TAG = "TransferAbortAsync";
		
		private String url;
		private String name;
		
		@Override
		protected Boolean doInBackground(String... params) {
			this.url = params[0];
			this.name = params[0];
			publishProgress("doInBackground() - TransferAbortAsync url: " + url + " Name: " + name);
			
			Boolean abort = rpc.transferOp(RpcClient.TRANSFER_ABORT, url, name);
			if(abort) {
				Log.d(TAG, "successful.");
			}
			return abort;
		}
		
		@Override
		protected void onPostExecute(Boolean success) {
			forceRefresh();
		}

		@Override
		protected void onProgressUpdate(String... arg0) {
			Log.d(TAG, "onProgressUpdate - " + arg0[0]);
		}
	}

	private final class TransferRetryAsync extends AsyncTask<String,String,Boolean> {

		private final String TAG = "TransferRetryAsync";
		
		private String url;
		private String name;
		
		@Override
		protected Boolean doInBackground(String... params) {
			this.url = params[0];
			this.name = params[1];
			publishProgress("doInBackground() - TransferRetryAsync url: " + url + " Name: " + name);
			
			Boolean retry = rpc.transferOp(RpcClient.TRANSFER_RETRY, url, name);
			if(retry) {
				publishProgress("successful.");
			}
			return retry;
		}
		
		@Override
		protected void onPostExecute(Boolean success) {
			forceRefresh();
		}

		@Override
		protected void onProgressUpdate(String... arg0) {
			Log.d(TAG, "onProgressUpdate - " + arg0[0]);
		}
	}

	private final class WriteClientRunModeAsync extends AsyncTask<Integer, String, Boolean> {

		private final String TAG = "WriteClientRunModeAsync";
		
		@Override
		protected Boolean doInBackground(Integer... params) {
			Boolean success = rpc.setRunMode(params[0], 0);
        	publishProgress("run mode set to " + params[0] + " returned " + success);
			return success;
		}
		
		@Override
		protected void onPostExecute(Boolean success) {
			forceRefresh();
		}

		@Override
		protected void onProgressUpdate(String... arg0) {
			Log.d(TAG, "onProgressUpdate - " + arg0[0]);
		}
	}
}
