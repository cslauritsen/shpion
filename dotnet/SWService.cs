using System;
using System.Diagnostics;
using System.ServiceProcess;
using System.Text;
using System.Xml;
using uPLibrary.Networking.M2Mqtt.Exceptions;
using uPLibrary.Networking.M2Mqtt.Messages;
using uPLibrary.Networking.M2Mqtt.Session;
using uPLibrary.Networking.M2Mqtt.Utility;
using uPLibrary.Networking.M2Mqtt.Internal;
using uPLibrary.Networking.M2Mqtt;

public class SWService : ServiceBase {
	private MqttClient client;
	private string server;

	public static void Main(string[] args) {
		 ServiceBase.Run(new SWService());
	}

	public SWService() {
		CanPauseAndContinue = true;
        	CanHandleSessionChangeEvent = true;
        	ServiceName = "SWService";
	}

	protected override void OnStart(string[] args) {
		Report("ServiceStart");
	}

	protected override void OnStop() {
		Report("ServiceStop");
		client.Disconnect();
	}

	protected override void OnShutdown() {
		Report("Shutdown");
		client.Disconnect();
	}

	private string getMqttServer() {
		if (server == null) {
			XmlDocument doc = new XmlDocument();
			doc.Load("c:\\pnp\\dotnet\\config.xml");
			XmlNode node = doc.DocumentElement.SelectSingleNode("/config/server");
			server = node.InnerText;
			EventLog.WriteEntry("Using mqtt server: " + server);
		}

		return server;
	}

	
	void client_MqttMsgPublishReceived(object sender, MqttMsgPublishEventArgs e) {
		string body = Encoding.UTF8.GetString(e.Message);
		//Console.WriteLine("Message received: \n" + e.Topic + ": " + body);
	}

	protected override void OnSessionChange(SessionChangeDescription changeDescription) {
		EventLog.WriteEntry("SWService.OnSessionChange", DateTime.Now.ToLongTimeString() +
			" - Session change notice received: " +
			changeDescription.Reason.ToString() + "  Session ID: " + 
			changeDescription.SessionId.ToString());


		switch (changeDescription.Reason) {
			case SessionChangeReason.SessionLock:
				EventLog.WriteEntry("SWService.OnSessionChange: Lock");
				Report(changeDescription.Reason.ToString());
				break;

			case SessionChangeReason.SessionUnlock:
				EventLog.WriteEntry("SWService.OnSessionChange: Unlock");
				Report(changeDescription.Reason.ToString());
				break;

			case SessionChangeReason.SessionLogon:
				EventLog.WriteEntry("SWService.OnSessionChange: Logon");
				Report(changeDescription.Reason.ToString());
				break;

			case SessionChangeReason.SessionLogoff:       
				EventLog.WriteEntry("SWService.OnSessionChange Logoff"); 
				Report(changeDescription.Reason.ToString());
				break;
			default:
				Report(changeDescription.Reason.ToString());
				break;
		}
	}

    
    /// <summary>
    /// The windows session has been locked
    /// </summary>
    protected void Report(string rsn) {
	Int32 unixTimestamp = (Int32)(DateTime.UtcNow.Subtract(new DateTime(1970, 1, 1))).TotalSeconds;
	string pubBody = unixTimestamp.ToString();

	if (client == null || !client.IsConnected) {
		// create client instance
		client = new MqttClient(getMqttServer());
		// register to message received
		client.MqttMsgPublishReceived += client_MqttMsgPublishReceived;
		string clientId = Guid.NewGuid().ToString();
		client.Connect(clientId);
	}

	client.Publish("shpion/session/" + rsn + "/" + Environment.GetEnvironmentVariable("COMPUTERNAME"), Encoding.UTF8.GetBytes(pubBody));
        return;
    }
}
