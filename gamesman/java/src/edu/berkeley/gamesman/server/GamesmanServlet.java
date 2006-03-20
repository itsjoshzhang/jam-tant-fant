package edu.berkeley.gamesman.server;

import java.io.*;
import javax.servlet.*;
import javax.servlet.http.*;

/**
 * The Servlet that handles the requests from the client and calls specific
 * modules to implement them
 * 
 * @author Amit Matani
 */

public class GamesmanServlet extends HttpServlet {

	private IModule modulesArray[];

	private boolean isServerAlive;

	/**
	 * Called only once when tomcat initializes the servlet. The ServletConfig
	 * holds the parameter values set in the web.xml file This will instantiate
	 * all of the modules and place them in the modules array. The class throws
	 * a servlet exception because it deals with the web.xml file and not the
	 * clients
	 * 
	 * The web.xml file must have the following init-params: imodules.lenghth
	 * imodules.list
	 * 
	 * @param config
	 *            config object of the web.xml file
	 * @throws ServletException
	 */
	public void init(ServletConfig config) throws ServletException {
		String iModuleName;
		String iModuleArgs;
		String iModuleArgsList[];
		int iModuleLength;

		try {
			iModuleLength = Integer.parseInt(config
					.getInitParameter("imodules.length"));
		} catch (Exception e) {
			throw new ServletException("Bad imodules.length value");
		}

		modulesArray = new IModule[iModuleLength];

		for (int i = 0; i < iModuleLength; i++) {
			if ((iModuleName = config.getInitParameter("imodules." + i
					+ ".classname")) == null)
				throw new ServletException(
						"imodule.length and list do not agree");
			else {
				try {
					Class newClass = Class.forName(iModuleName);
					modulesArray[i] = (IModule) newClass.newInstance();
				} catch (Exception e) {
					throw new ServletException("Bad Module Name");
				}
				if ((iModuleArgs = config.getInitParameter("imodules." + i
						+ ".args")) != null) {
					iModuleArgsList = iModuleArgs.split(",");
				} else {
					iModuleArgsList = new String[0];
				}
				try {
					modulesArray[i].initialize(iModuleArgsList);
				} catch (ModuleInitializationException e) {
					System.out.println("Module Initialization Error:"
							+ modulesArray.getClass().getName());
				}
			}
		}
		isServerAlive = true;
	}

	/**
	 * The doPost function will respond to a post request from the client. The
	 * client will package the request and reponse in a ModuleRequest and
	 * ModuleResponse and calls the appropriate module to handle it. It also
	 * handles servlet exceptions thrown from any of the modules and throws them
	 * back to the client
	 * 
	 * @param config
	 *            config object of the web.xml file
	 * @throws ServletException
	 */
	public void doPost(HttpServletRequest request, HttpServletResponse response)
			throws IOException, ServletException {
		ModuleRequest mRequest = new ModuleRequest(request);
		ModuleResponse mResponse = new ModuleResponse(response);
		int counter = 0;
		if (isServerAlive) {
			while (counter < modulesArray.length
					&& !modulesArray[counter].typeSupported(mRequest
							.getHeader("type"))) {
				counter++;
			}
			if (counter < modulesArray.length) {
				try {
					modulesArray[counter].handleRequest(mRequest, mResponse);
				} catch (ModuleException e) {
					// send back error code
					mResponse.setHeader("error_code", String.valueOf(e
							.getCode()));
					mResponse.setHeader("error_message", e.getMessage());
				}
			} else {
				mResponse.setHeader("error_code", String
						.valueOf(ModuleException.BAD_REQUEST_TYPE));
				mResponse.setHeader("error_message", "Bad Request Type");
			}
		} else {
			mResponse.setHeader("error_code", String
					.valueOf(ModuleException.BAD_REQUEST_TYPE));
			mResponse.setHeader("error_message", "Server is Down");
		}
	}

	public void doGet(HttpServletRequest request, HttpServletResponse response)
			throws IOException, ServletException {

		return;
	}
}