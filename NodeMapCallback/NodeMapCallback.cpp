//=============================================================================
// Copyright (c) 2025 FLIR Integrated Imaging Solutions, Inc. All Rights Reserved.
//
// This software is the confidential and proprietary information of FLIR
// Integrated Imaging Solutions, Inc. ("Confidential Information"). You
// shall not disclose such Confidential Information and shall use it only in
// accordance with the terms of the license agreement you entered into
// with FLIR Integrated Imaging Solutions, Inc. (FLIR).
//
// FLIR MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF THE
// SOFTWARE, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE, OR NON-INFRINGEMENT. FLIR SHALL NOT BE LIABLE FOR ANY DAMAGES
// SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR DISTRIBUTING
// THIS SOFTWARE OR ITS DERIVATIVES.
//=============================================================================

/**
 *  @example NodeMapCallback.cpp
 *
 *  @brief NodeMapCallback.cpp shows how to use nodemap callbacks. It relies
 *  on information provided in the Enumeration, Acquisition, and NodeMapInfo
 *  examples. As callbacks are very similar to events, it may be a good idea to
 *  explore this example prior to tackling the events examples.
 *
 *  This example focuses on creating, registering, using, and unregistering
 *  callbacks. A callback requires a function signature, which allows it to be
 *  registered to and access a node. Events, while slightly more complex,
 *  follow this same pattern.
 *
 *  Once comfortable with NodeMapCallback, we suggest checking out any of the
 *  events examples: EnumerationEvents, ImageEvents, or Logging.
 *
 *  Please leave us feedback at: https://www.surveymonkey.com/r/TDYMVAPI
 *  More source code examples at: https://github.com/Teledyne-MV/Spinnaker-Examples
 *  Need help? Check out our forum at: https://teledynevisionsolutions.zendesk.com/hc/en-us/community/topics
 */

#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
#include <iostream>
#include <sstream>

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace std;

// This is the first of three callback functions. Notice the function signature.
// This callback function will be registered to the height node.
void OnHeightNodeUpdate(INode* node)
{
    CIntegerPtr ptrHeight = node;

    if (GenApi::IsReadable(ptrHeight))
    {
        cout << "Height callback message:" << endl;
        cout << "\tLook! Height changed to " << ptrHeight->GetValue() << "..." << endl << endl;
    }
    else
    {
        cout << "Height callback triggered but node is not readable..." << endl;
    }
}

// This is the second of three callback functions. Notice that despite different
// names, everything else is exactly the same as the first. This callback
// function will be registered to the gain node.
void OnGainNodeUpdate(INode* node)
{
    CFloatPtr ptrGain = node;

    if (GenApi::IsReadable(ptrGain))
    {
        cout << "Gain callback message:" << endl;
        cout << "\tLook now!  Gain changed to " << ptrGain->GetValue() << "..." << endl << endl;
    }
    else
    {
        cout << "Gain callback triggered but node is not readable..." << endl;
    }
}

// This is the third of three callback functions. Notice the function signature.
// This callback function will be registered to the event feature nodes.
void OnEventNodeUpdate(INode* node)
{
    CNodePtr ptrEventNode = node;

    GenApi::EInterfaceType nodeType = ptrEventNode->GetPrincipalInterfaceType();
    if (!IsReadable(ptrEventNode))
    {
        cout << ptrEventNode->GetName() << " with node type " << nodeType << " was updated" << endl;
        return;
    }

    // Handle common event data types
    switch (nodeType)
    {
    case intfIInteger:
        cout << ptrEventNode->GetName() << " was changed to " << CIntegerPtr(node)->GetValue() << endl;
        break;
    case intfIBoolean:
        cout << ptrEventNode->GetName() << " was changed to " << CBooleanPtr(node)->GetValue() << endl;
        break;
    case intfIFloat:
        cout << ptrEventNode->GetName() << " was changed to " << CFloatPtr(node)->GetValue() << endl;
        break;
    case intfIString:
        cout << ptrEventNode->GetName() << " was changed to " << CStringPtr(node)->GetValue() << endl;
        break;
    default:
        cout << ptrEventNode->GetName() << " with node type " << nodeType << " was updated" << endl;
        break;
    }
}

// This function prepares the example by disabling automatic gain, creating the
// callbacks, and registering them to their respective nodes.
int ConfigureCallbacks(INodeMap& nodeMap, std::vector<CallbackHandleType>& callbackHandles)
{
    int result = 0;

    cout << endl << endl << "*** CONFIGURING CALLBACKS ***" << endl << endl;

    try
    {
        //
        // Turn off automatic gain
        //
        // *** NOTES ***
        // Automatic gain prevents the manual configuration of gain and needs to
        // be turned off for this example.
        //
        // *** LATER ***
        // Automatic exposure is turned off at the end of the example in order
        // to restore the camera to its default state.
        
        CEnumerationPtr ptrGainAuto = nodeMap.GetNode("GainAuto");
        if (IsReadable(ptrGainAuto) &&
            IsWritable(ptrGainAuto))
        {
            CEnumEntryPtr ptrGainAutoOff = ptrGainAuto->GetEntryByName("Off");
            if (!IsReadable(ptrGainAutoOff))
            {
                cout << "Unable to disable automatic gain (enum entry retrieval). Aborting..." << endl << endl;
                return -1;
            }

            ptrGainAuto->SetIntValue(ptrGainAutoOff->GetValue());

            cout << "Automatic gain disabled..." << endl;
        }
        else
        {
            CEnumerationPtr ptrAutoBright = nodeMap.GetNode("autoBrightnessMode");
            // If autobrightness exists, auto gain doesnt need to be disabled
            if (!IsReadable(ptrAutoBright) ||
                !IsWritable(ptrAutoBright))
            {
                cout << "Unable to disable automatic gain (node retrieval). Expected for some models..." << endl << endl;
                result = 1;
            }
            else
            {
                cout << "Skipping automatic gain disabling... Expected for some models..." << endl;
                result = 1;
            }
        }


        //
        // Register callback to height node
        //
        // *** NOTES ***
        // Callbacks need to be registered to nodes, which should be writable
        // if the callback is to ever be triggered. Notice that callback
        // registration returns an integer - this integer is important at the
        // end of the example for deregistration.
        //
        // *** LATER ***
        // Each callback needs to be unregistered individually before releasing
        // the system or an exception will be thrown.
        //
        CIntegerPtr ptrHeight = nodeMap.GetNode("Height");
        if (!IsWritable(ptrHeight))
        {
            cout << "Unable to retrieve height. Aborting..." << endl << endl;
            return -1;
        }

        cout << "Height ready..." << endl;

        CallbackHandleType callbackHeight = Register(ptrHeight, &OnHeightNodeUpdate);
        callbackHandles.push_back(callbackHeight);

        cout << "Height callback registered..." << endl;

        //
        // Register callback to gain node
        //
        // *** NOTES ***
        // Depending on the specific goal of the function, it can be important
        // to notice the node type that a callback is registered to. Notice in
        // the callback functions above that the callback registered to height
        // casts its node as an integer whereas the callback registered to gain
        // casts as a float.
        //
        // *** LATER ***
        // Each callback needs to be unregistered individually before releasing
        // the system or an exception will be thrown.
        //
        CFloatPtr ptrGain = nodeMap.GetNode("Gain");
        if (!IsWritable(ptrGain))
        {
            cout << "Unable to retrieve gain. Aborting..." << endl << endl;
            return -1;
        }

        cout << "Gain ready..." << endl;

        CallbackHandleType callbackGain = Register(ptrGain, &OnGainNodeUpdate);
        callbackHandles.push_back(callbackGain);

        cout << "Gain callback registered..." << endl << endl;
    }
    catch (Spinnaker::Exception& e)
    {
        cout << "Error: " << e.what() << endl;
        result = -1;
    }

    return result;
}


// This function enables all available node events available in the EventSelector, and then creates
// and registers genicam node callback for each related node event data.
int ConfigureEventCallbacks(INodeMap& nodeMap, std::vector<CallbackHandleType>& callbackHandles)
{
    int result = 0;

    cout << endl << endl << "*** CONFIGURING EVENT CALLBACKS ***" << endl << endl;

    try
    {
        //
        // Retrieve event selector
        //
        // *** NOTES ***
        // Each type of event must be enabled individually. This is done
        // by retrieving "EventSelector" (an enumeration node) and then enabling
        // the specific event on "EventNotification" (another enumeration node).
        //
        CEnumerationPtr ptrEventSelector = nodeMap.GetNode("EventSelector");
        if (!IsReadable(ptrEventSelector) || !IsWritable(ptrEventSelector))
        {
            cout << "Unable to retrieve event selector entries. Skipping..." << endl << endl;
            return 1;
        }

        NodeList_t entries;
        ptrEventSelector->GetEntries(entries);

        cout << "Enabling event selector entries..." << endl;

        //
        // Enable device events
        //
        // *** NOTES ***
        // In order to enable a specific event, the event selector and event
        // notification nodes (both of type enumeration) must work in unison.
        // The desired event must first be selected on the event selector node
        // and then enabled on the event notification node.
        //
        for (unsigned int i = 0; i < entries.size(); i++)
        {
            // Select entry on selector node
            CEnumEntryPtr ptrEnumEntry = entries.at(i);
            if (!IsReadable(ptrEnumEntry))
            {
                // Skip if node fails
                continue;
            }

            ptrEventSelector->SetIntValue(ptrEnumEntry->GetValue());

            // Retrieve event notification node (an enumeration node)
            CEnumerationPtr ptrEventNotification = nodeMap.GetNode("EventNotification");

            // Retrieve entry node to enable device event
            if (!IsReadable(ptrEventNotification))
            {
                // Skip if node fails
                continue;
            }

            CEnumEntryPtr ptrEventNotificationOn = ptrEventNotification->GetEntryByName("On");

            if (!IsReadable(ptrEventNotificationOn))
            {
                // Skip if node fails
                continue;
            }

            if (!IsWritable(ptrEventNotification))
            {
                // Skip if node fails
                continue;
            }

            ptrEventNotification->SetIntValue(ptrEventNotificationOn->GetValue());

            cout << "\t" << ptrEnumEntry->GetDisplayName() << ": enabled..." << endl;

            // Register Event Data callbacks
            auto eventDataCategoryName = "Event" + ptrEnumEntry->GetSymbolic() + "Data";
            CCategoryPtr ptrDataCategory = nodeMap.GetNode(eventDataCategoryName);

            if (ptrDataCategory)
            {
                GenApi::FeatureList_t features;
                ptrDataCategory->GetFeatures(features);

                for (const auto& it : features)
                {
                    //
                    // Register callback to event data node
                    //
                    // *** LATER ***
                    // Each callback needs to be unregistered individually before releasing
                    // the system or an exception will be thrown.
                    //
                    CNodePtr ptrNode = it->GetNode();
                    CallbackHandleType callbackHandle = Register(ptrNode, &OnEventNodeUpdate);
                    callbackHandles.push_back(callbackHandle);

                    cout << "\t\t" << ptrNode->GetName() << " callback registered..." << endl;
                }
            }
        }
    }
    catch (Spinnaker::Exception& e)
    {
        cout << "Error: " << e.what() << endl;
        result = -1;
    }

    return result;
}

// This function demonstrates the triggering of the nodemap callbacks. First it
// changes height, which executes the callback registered to the height node, and
// then it changes gain, which executes the callback registered to the gain node.
int ChangeHeightAndGain(INodeMap& nodeMap)
{
    int result = 0;

    cout << endl << "*** CHANGE HEIGHT & GAIN ***" << endl << endl;

    try
    {
        //
        // Change height to trigger height callback
        //
        // *** NOTES ***
        // Notice that changing the height only triggers the callback function
        // registered to the height node.
        //
        CIntegerPtr ptrHeight = nodeMap.GetNode("Height");
        if (!IsReadable(ptrHeight) || !IsWritable(ptrHeight) || ptrHeight->GetInc() == 0 || ptrHeight->GetMax() == 0)
        {
            cout << "Unable to retrieve height. Aborting..." << endl << endl;
            return -1;
        }

        int64_t heightToSet = ptrHeight->GetMax();

        cout << "Regular function message:" << endl;
        cout << "\tHeight about to be changed to " << heightToSet << "..." << endl << endl;

        ptrHeight->SetValue(heightToSet);

        //
        // Change gain to trigger gain callback
        //
        // *** NOTES ***
        // The same is true of changing the gain node; changing a node will
        // only ever trigger the callback function (or functions) currently
        // registered to it.
        //
        CFloatPtr ptrGain = nodeMap.GetNode("Gain");
        if (!IsReadable(ptrGain) || !IsWritable(ptrGain) || ptrGain->GetMax() == 0)
        {
            cout << "Unable to retrieve gain..." << endl;
            return -1;
        }

        double gainToSet = ptrGain->GetMax() / 2.0;

        cout << "Regular function message:" << endl;
        cout << "\tGain about to be changed to " << gainToSet << "..." << endl << endl;

        ptrGain->SetValue(gainToSet);
    }
    catch (Spinnaker::Exception& e)
    {
        cout << "Error: " << e.what() << endl;
        result = -1;
    }

    return result;
}

// This function cleans up the example by deregistering the callbacks and
// turning automatic gain back on.
int ResetCallbacks(std::vector<CallbackHandleType>& callbackHandles)
{
    int result = 0;

    try
    {
        //
        // Deregister callbacks
        //
        // *** NOTES ***
        // It is important to deregister each callback function from each node
        // that it is registered to.
        //
        for (CallbackHandleType handle : callbackHandles)
        {
            Deregister(handle);
        }

        cout << "Callbacks deregistered..." << endl << endl;
    }
    catch (Spinnaker::Exception& e)
    {
        cout << "Error: " << e.what() << endl;
        result = -1;
    }

    return result;
}

// This function cleans up the example by deregistering the callbacks and
// turning event notification off
int ResetEvents(INodeMap& nodeMap)
{
    int result = 0;

    try
    {
        //
        // Disable event notifications
        //
        CEnumerationPtr ptrEventSelector = nodeMap.GetNode("EventSelector");
        if (!IsReadable(ptrEventSelector) || !IsWritable(ptrEventSelector))
        {
            cout << "Unable to retrieve event selector entries. Skipping..." << endl << endl;
            return 0;
        }

        NodeList_t entries;
        ptrEventSelector->GetEntries(entries);

        cout << "Disabling event selector entries..." << endl;

        for (unsigned int i = 0; i < entries.size(); i++)
        {
            // Select entry on selector node
            CEnumEntryPtr ptrEnumEntry = entries.at(i);
            if (!IsReadable(ptrEnumEntry))
            {
                // Skip if node fails
                continue;
            }

            ptrEventSelector->SetIntValue(ptrEnumEntry->GetValue());

            // Retrieve event notification node (an enumeration node)
            CEnumerationPtr ptrEventNotification = nodeMap.GetNode("EventNotification");

            // Retrieve entry node to enable device event
            if (!IsReadable(ptrEventNotification))
            {
                // Skip if node fails
                result = -1;
                continue;
            }
            CEnumEntryPtr ptrEventNotificationOn = ptrEventNotification->GetEntryByName("Off");

            if (!IsReadable(ptrEventNotificationOn))
            {
                // Skip if node fails
                result = -1;
                continue;
            }

            if (!IsWritable(ptrEventNotification))
            {
                // Skip if node fails
                result = -1;
                continue;
            }
            ptrEventNotification->SetIntValue(ptrEventNotificationOn->GetValue());

            cout << "\t" << ptrEnumEntry->GetDisplayName() << ": disabled..." << endl;
        }
    }
    catch (Spinnaker::Exception& e)
    {
        cout << "Error: " << e.what() << endl;
        result = -1;
    }

    return result;
}

int ResetAutoGain(INodeMap& nodeMap)
{
    //
    // Turn automatic gain back on
    //
    // *** NOTES ***
    // Automatic gain is turned back on in order to restore the camera to
    // its default state.
    //
    int result = 0;
    try
    {
        CEnumerationPtr ptrGainAuto = nodeMap.GetNode("GainAuto");
        if (!IsReadable(ptrGainAuto) ||
            !IsWritable(ptrGainAuto))
        {
            cout << "Unable to enable automatic gain (node retrieval). Non-fatal error..." << endl << endl;
            return -1;
        }

        CEnumEntryPtr ptrGainAutoContinuous = ptrGainAuto->GetEntryByName("Continuous");
        if (!IsReadable(ptrGainAutoContinuous))
        {
            cout << "Unable to enable automatic gain (enum entry retrieval). Non-fatal error..." << endl << endl;
            return -1;
        }

        ptrGainAuto->SetIntValue(ptrGainAutoContinuous->GetValue());

        cout << "Automatic gain enabled..." << endl << endl;
    }
    catch (Spinnaker::Exception& e)
    {
        cout << "Error: " << e.what() << endl;
        result = -1;
    }
    return result;
}

// This function prints the device information of the camera from the transport
// layer; please see NodeMapInfo example for more in-depth comments on printing
// device information from the nodemap.
int PrintDeviceInfo(INodeMap& nodeMap)
{
    int result = 0;
    cout << endl << "*** DEVICE INFORMATION ***" << endl << endl;

    try
    {
        FeatureList_t features;
        CCategoryPtr category = nodeMap.GetNode("DeviceInformation");
        if (IsReadable(category))
        {
            category->GetFeatures(features);

            FeatureList_t::const_iterator it;
            for (it = features.begin(); it != features.end(); ++it)
            {
                try
                {
                    CNodePtr pfeatureNode = *it;
                    cout << pfeatureNode->GetName() << " : ";
                    CValuePtr pValue = (CValuePtr)pfeatureNode;
                    cout << (IsReadable(pValue) ? pValue->ToString() : "Node not readable");
                    cout << endl;
                }
                catch (Spinnaker::Exception)
                {
                    cout << "Node not readable" << endl;
                }
            }
        }
        else
        {
            cout << "Device control information not readable." << endl;
        }
    }
    catch (Spinnaker::Exception& e)
    {
        cout << "Error: " << e.what() << endl;
        result = -1;
    }

    return result;
}

// This function acquires 10 images from a device to trigger acquisition related 
// nodemap events; please see Acquisition example for more in-depth comments on
// acquiring images.
int AcquireImages(CameraPtr pCam, INodeMap& nodeMap, INodeMap& nodeMapTLDevice)
{
    int result = 0;

    cout << endl << "*** IMAGE ACQUISITION ***" << endl << endl;

    try
    {
        // Set acquisition mode to continuous
        CEnumerationPtr ptrAcquisitionMode = nodeMap.GetNode("AcquisitionMode");
        if (!IsReadable(ptrAcquisitionMode) || !IsWritable(ptrAcquisitionMode))
        {
            cout << "Unable to get or set acquisition mode to continuous (node retrieval). Aborting..." << endl << endl;
            return -1;
        }

        CEnumEntryPtr ptrAcquisitionModeContinuous = ptrAcquisitionMode->GetEntryByName("Continuous");
        if (!IsReadable(ptrAcquisitionModeContinuous))
        {
            cout << "Unable to get acquisition mode to continuous (entry 'continuous' retrieval). Aborting..." << endl
                 << endl;
            return -1;
        }

        int64_t acquisitionModeContinuous = ptrAcquisitionModeContinuous->GetValue();

        ptrAcquisitionMode->SetIntValue(acquisitionModeContinuous);

        cout << "Acquisition mode set to continuous..." << endl;

        // Begin acquiring images
        pCam->BeginAcquisition();

        cout << "Acquiring images..." << endl;

        // Retrieve device serial number for filename
        gcstring deviceSerialNumber("");

        CStringPtr ptrStringSerial = nodeMapTLDevice.GetNode("DeviceSerialNumber");
        if (IsReadable(ptrStringSerial))
        {
            deviceSerialNumber = ptrStringSerial->GetValue();

            cout << "Device serial number retrieved as " << deviceSerialNumber << "..." << endl;
        }
        cout << endl;

        // Retrieve and convert images
        const unsigned int k_numImages = 10;

        //
        // Create ImageProcessor instance for post processing images
        //
        ImageProcessor processor;

        //
        // Set default image processor color processing method
        //
        // *** NOTES ***
        // By default, if no specific color processing algorithm is set, the image
        // processor will default to NEAREST_NEIGHBOR method.
        //
        processor.SetColorProcessing(SPINNAKER_COLOR_PROCESSING_ALGORITHM_HQ_LINEAR);

        for (unsigned int imageCnt = 0; imageCnt < k_numImages; imageCnt++)
        {
            try
            {
                // Retrieve next received image and ensure image completion
                ImagePtr pResultImage = pCam->GetNextImage(1000);

                if (pResultImage->IsIncomplete())
                {
                    cout << "Image incomplete with image status " << pResultImage->GetImageStatus() << "..." << endl
                         << endl;
                }
                else
                {
                    // Print image information
                    cout << "Grabbed image " << imageCnt << ", width = " << pResultImage->GetWidth()
                         << ", height = " << pResultImage->GetHeight() << endl;
                }

                // Release image
                pResultImage->Release();

                cout << endl;
            }
            catch (Spinnaker::Exception& e)
            {
                cout << "Error: " << e.what() << endl;
                result = -1;
            }
        }

        // End acquisition
        pCam->EndAcquisition();
    }
    catch (Spinnaker::Exception& e)
    {
        cout << "Error: " << e.what() << endl;
        result = -1;
    }

    return result;
}

// This function acts as the body of the example; please see NodeMapInfo example
// for more in-depth comments on setting up cameras.
int RunSingleCamera(CameraPtr pCam)
{
    int result = 0;
    int err = 0;

    try
    {
        // Initialize camera
        pCam->Init();

        // Retrieve GenICam nodemap
        INodeMap& nodeMap = pCam->GetNodeMap();

        // Retrieve TL device nodemap and print device information
        INodeMap& nodeMapTLDevice = pCam->GetTLDeviceNodeMap();

        // Retrieve TL stream nodemap
        INodeMap& nodeMapTLStream = pCam->GetTLStreamNodeMap();

        result = PrintDeviceInfo(nodeMapTLDevice);

        std::vector<CallbackHandleType> callbacks;

        // Configure regular node callbacks
        err = ConfigureCallbacks(nodeMap, callbacks);
        if (err < 0)
        {
            return err;
        }

        // Configure event callbacks on remote device
        err = ConfigureEventCallbacks(nodeMap, callbacks);
        if (err < 0)
        {
            return err;
        }

        // Configure event callbacks on local device
        err = ConfigureEventCallbacks(nodeMapTLDevice, callbacks);
        if (err < 0)
        {
            return err;
        }

        // Configure event callbacks on local stream
        err = ConfigureEventCallbacks(nodeMapTLStream, callbacks);
        if (err < 0)
        {
            return err;
        }

        // Change height and gain to trigger callbacks
        result = result | ChangeHeightAndGain(nodeMap);

        // Acquire image to trigger event callbacks
        result = result | AcquireImages(pCam, nodeMap, nodeMapTLDevice);

        // Reset callbacks
        result = result | ResetCallbacks(callbacks);

        // Only reset automatic gain if we changed it
        if (err == 0) 
        {
            result = result | ResetAutoGain(nodeMap);
        }

        // Reset events
        result = result | ResetEvents(nodeMap);
        result = result | ResetEvents(nodeMapTLDevice);
        result = result | ResetEvents(nodeMapTLStream);

        // Deinitialize camera
        pCam->DeInit();
    }
    catch (Spinnaker::Exception& e)
    {
        cout << "Error: " << e.what() << endl;
        result = -1;
    }

    return result;
}

// Example entry point; please see Enumeration example for more in-depth
// comments on preparing and cleaning up the system.
int main(int /*argc*/, char** /*argv*/)
{
    int result = 0;

    // Print application build information
    cout << "Application build date: " << __DATE__ << " " << __TIME__ << endl << endl;

    // Retrieve singleton reference to system object
    SystemPtr system = System::GetInstance();

    // Print out current library version
    const LibraryVersion spinnakerLibraryVersion = system->GetLibraryVersion();
    cout << "Spinnaker library version: " << spinnakerLibraryVersion.major << "." << spinnakerLibraryVersion.minor
         << "." << spinnakerLibraryVersion.type << "." << spinnakerLibraryVersion.build << endl
         << endl;

    // Retrieve list of cameras from the system
    CameraList camList = system->GetCameras();

    unsigned int numCameras = camList.GetSize();

    cout << "Number of cameras detected: " << numCameras << endl << endl;

    // Finish if there are no cameras
    if (numCameras == 0)
    {
        // Clear camera list before releasing system
        camList.Clear();

        // Release system
        system->ReleaseInstance();

        cout << "Not enough cameras!" << endl;
        cout << "Done! Press Enter to exit..." << endl;
        getchar();

        return -1;
    }

    // Run example on each camera
    for (unsigned int i = 0; i < numCameras; i++)
    {
        cout << endl << "Running example for camera " << i << "..." << endl;

        result = result | RunSingleCamera(camList.GetByIndex(i));

        cout << "Camera " << i << " example complete..." << endl << endl;
    }

    // Clear camera list before releasing system
    camList.Clear();

    // Release system
    system->ReleaseInstance();

    cout << endl << "Done! Press Enter to exit..." << endl;
    getchar();

    return result;
}
