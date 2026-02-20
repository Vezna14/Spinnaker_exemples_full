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
 *  @example ChunkData_C.c
 *
 *  @brief ChunkData_C.c shows how to get chunk data on an image, either from
 *  the nodemap or from the image itself. It relies on information provided in
 *  the Enumeration_C, Acquisition_C, and NodeMapInfo_C examples.
 *
 *  It can also be helpful to familiarize yourself with the ImageFormatControl_C
 *  and Exposure_C examples. As they are somewhat shorter and simpler, either
 *  provides a strong introduction to camera customization.
 *
 *  Chunk data provides information on various traits of an image. This includes
 *  identifiers such as frame ID, properties such as black level, and more. This
 *  information can be acquired from either the nodemap or the image itself.
 *
 *  It may be preferable to grab chunk data from each individual image, as it
 *  can be hard to verify whether data is coming from the correct image when
 *  using the nodemap. This is because chunk data retrieved from the nodemap is
 *  only valid for the current image; when spinCameraGetNextImage() or
 *  spinCameraGetNextImageEx() is called, chunk data will be updated to that of
 *  the new current image.
 *
 *  Please leave us feedback at: https://www.surveymonkey.com/r/TDYMVAPI
 *  More source code examples at: https://github.com/Teledyne-MV/Spinnaker-Examples
 *  Need help? Check out our forum at: https://teledynevisionsolutions.zendesk.com/hc/en-us/community/topics
 */

#include "SpinnakerC.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

// This macro helps with C-strings.
#define MAX_BUFF_LEN 256

char lastErrorMessage[MAX_BUFF_LEN];
size_t lenLastErrorMessage = MAX_BUFF_LEN;

// Helper for getting error messages
char* GetLastErrorMessage()
{
    // Note: lastErrorMessage is shared across multiple threads; a different thread could overwrite the last error
    // message before this function is called to grab the latest message
    spinErrorGetLastMessage(lastErrorMessage, &lenLastErrorMessage);
    return lastErrorMessage;
}

// This function helps to check if a node is readable
bool8_t IsReadable(spinNodeHandle hNode, char nodeName[])
{
    spinError err = SPINNAKER_ERR_SUCCESS;
    bool8_t pbReadable = False;
    err = spinNodeIsReadable(hNode, &pbReadable);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf(
            "Unable to retrieve node readability (%s node) with error: %s [%d]\n\n",
            nodeName,
            GetLastErrorMessage(),
            err);
    }
    return pbReadable;
}

// This function helps to check if a node is writable
bool8_t IsWritable(spinNodeHandle hNode, char nodeName[])
{
    spinError err = SPINNAKER_ERR_SUCCESS;
    bool8_t pbWritable = False;
    err = spinNodeIsWritable(hNode, &pbWritable);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf(
            "Unable to retrieve node writability (%s node) with error: %s [%d]\n\n",
            nodeName,
            GetLastErrorMessage(),
            err);
    }
    return pbWritable;
}

// Use the following enum and global constant to select whether chunk data is
// displayed from the image or the nodemap.
typedef enum _chunkDataType
{
    IMAGE,
    NODEMAP
} chunkDataType;

const chunkDataType chosenChunkData = IMAGE;

// This function configures the camera to add chunk data to each image. It does
// this by enabling each type of chunk data after enabling chunk data mode.
// When chunk data mode is turned on, the data is made available in both the nodemap
// and each image.
spinError ConfigureChunkData(spinNodeMapHandle hNodeMap)
{
    spinError err = SPINNAKER_ERR_SUCCESS;

    unsigned int i = 0;

    printf("\n\n*** CONFIGURING CHUNK DATA ***\n\n");

    //
    // Activate chunk mode
    //
    // *** NOTES ***
    // Once enabled, chunk data will be available at the end of hte payload of
    // every image captured until it is disabled. Chunk data can also be
    // retrieved from the nodemap.
    //
    spinNodeHandle hChunkModeActive = NULL;

    err = spinNodeMapGetNode(hNodeMap, "ChunkModeActive", &hChunkModeActive);

    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf("Unable to activate chunk mode. Aborting with error: %s [%d]\n\n", GetLastErrorMessage(), err);
        return err;
    }

    // Check if available and writable
    if (IsWritable(hChunkModeActive, "ChunkModeActive"))
    {
        err = spinBooleanSetValue(hChunkModeActive, True);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf("Unable to activate chunk mode. Aborting with error: %s [%d]\n\n", GetLastErrorMessage(), err);
            return err;
        }
    }
    else
    {
        err = SPINNAKER_ERR_ACCESS_DENIED;
        printf("Unable to write to chunk mode. Aborting with error: %s [%d]\n\n", GetLastErrorMessage(), err);
        return err;
    }

    printf("Chunk mode activated...\n");

    //
    // Enable all types of chunk data
    //
    // *** NOTES ***
    // Enabling chunk data requires working with nodes: "ChunkSelector" is an
    // enumeration selector node and "ChunkEnable" is a boolean. It requires
    // retrieving the selector node (which is of enumeration node type),
    // selecting the entry of the chunk data to be enabled, retrieving the
    // corresponding boolean, and setting it to true.
    //
    // In this example, all chunk data is enabled, so these steps are performed
    // in a loop. Once this is complete, chunk mode still needs to be activated.
    //
    spinNodeHandle hChunkSelector = NULL;
    size_t numEntries = 0;

    // Retrieve selector node, check if available and readable and writable
    err = spinNodeMapGetNode(hNodeMap, "ChunkSelector", &hChunkSelector);

    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf(
            "Unable to retrieve chunk selector entries. Aborting with error: %s [%d]\n\n", GetLastErrorMessage(), err);
        return err;
    }

    // Retrieve number of entries
    if (IsReadable(hChunkSelector, "ChunkSelector"))
    {
        err = spinEnumerationGetNumEntries(hChunkSelector, &numEntries);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf(
                "Unable to retrieve number of entries. Aborting with error: %s [%d]\n\n", GetLastErrorMessage(), err);
            return err;
        }
    }
    else
    {
        err = SPINNAKER_ERR_ACCESS_DENIED;
        printf("Unable to read number of entries. Aborting with error: %s [%d]\n\n", GetLastErrorMessage(), err);
        return err;
    }

    printf("Enabling entries...\n");

    for (i = 0; i < numEntries; i++)
    {
        // Retrieve entry node
        spinNodeHandle hEntry = NULL;

        err = spinEnumerationGetEntryByIndex(hChunkSelector, i, &hEntry);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf("\tUnable to enable chunk entry (error %d)...\n\n", err);
            continue;
        }

        // Check if readable, retrieve entry name
        char entryName[MAX_BUFF_LEN];
        size_t lenEntryName = MAX_BUFF_LEN;

        if (IsReadable(hEntry, "ChunkEntry"))
        {
            err = spinNodeGetDisplayName(hEntry, entryName, &lenEntryName);
            if (err != SPINNAKER_ERR_SUCCESS)
            {
                printf("\t%s: unable to retrieve chunk entry display name (error %d)...\n", entryName, err);
            }
        }
        else
        {
            continue;
        }
        // Retrieve enum entry integer value
        int64_t value = 0;

        err = spinEnumerationEntryGetIntValue(hEntry, &value);

        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf("\t%s: unable to get chunk entry value (error %d)...\n", entryName, err);
            continue;
        }

        // Set integer value
        if (IsWritable(hChunkSelector, "ChunkSelector"))
        {
            err = spinEnumerationSetIntValue(hChunkSelector, value);
            if (err != SPINNAKER_ERR_SUCCESS)
            {
                printf("\t%s: unable to set chunk entry value (error %d)...\n", entryName, err);
                continue;
            }
        }
        else
        {
            err = SPINNAKER_ERR_ACCESS_DENIED;
            printf("\t%s: unable to write to chunk entry value (error %d)...\n", entryName, err);
            return err;
        }

        // Retrieve corresponding chunk enable node
        spinNodeHandle hChunkEnable = NULL;

        err = spinNodeMapGetNode(hNodeMap, "ChunkEnable", &hChunkEnable);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf("\t%s: unable to get entry from nodemap (error %d)...\n", entryName, err);
            continue;
        }

        // Retrieve chunk enable value and set to true if necessary
        bool8_t isEnabled = False;
        if (IsReadable(hChunkEnable, "ChunkEnable"))
        {
            err = spinBooleanGetValue(hChunkEnable, &isEnabled);
            if (err != SPINNAKER_ERR_SUCCESS)
            {
                printf("\t%s: unable to get chunk entry boolean value (error %d)...\n", entryName, err);
                continue;
            }
        }
        else
        {
            printf("\t%s: not writable\n", entryName);
            continue;
        }

        // Consider the case in which chunk data is enabled but not writable
        if (!isEnabled || !IsWritable(hChunkEnable, "ChunkEnable"))
        {
            // Set chunk enable value to true

            err = spinBooleanSetValue(hChunkEnable, True);
            if (err != SPINNAKER_ERR_SUCCESS)
            {
                printf("\t%s: unable to set chunk entry boolean value (error %d)...\n", entryName, err);
                continue;
            }
        }

        printf("\t%s: enabled\n", entryName);
    }

    return err;
}

void printBytesAsHex(uint8_t* array, int64_t length)
{
    printf("\n\t\t");
    for (int64_t i = 0; i < length - 1; ++i)
    {
        if (i != 0 && i % 8 == 0)
        {
            printf("\n\t\t");
        }
        printf("0x%02x ", (unsigned int)(array[i]));
    }
    printf("\n");
}

// This function displays a select amount of chunk data from the image. Unlike
// accessing chunk data via the nodemap, there is no way to loop through all
// available data.
spinError DisplayChunkDataFromImage(spinImage hImage)
{
    spinError err = SPINNAKER_ERR_SUCCESS;

    printf("Print chunk data from image...\n");

    //
    // Retrieve exposure time; exposure time recorded in microseconds
    //
    // *** NOTES ***
    // Floating point numbers are returned as a double
    //
    double exposureTime = 0.0;

    err = spinImageChunkDataGetFloatValue(hImage, "ChunkExposureTime", &exposureTime);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf(
            "Unable to retrieve exposure time from image chunk data. Aborting with error: %s [%d]\n\n",
            GetLastErrorMessage(),
            err);
        return err;
    }

    printf("\tExposure time: %f\n", exposureTime);

    //
    // Retrieve compression ratio
    //
    // *** NOTES ***
    // Floating point numbers are returned as a double
    //
    double compressionRatio = 0.0;

    err = spinImageChunkDataGetFloatValue(hImage, "ChunkCompressionRatio", &compressionRatio);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf(
            "Unable to retrieve compression ratio from image chunk data. Aborting with error: %s [%d]\n\n",
            GetLastErrorMessage(),
            err);
        return err;
    }

    printf("\tCompression ratio: %f\n", compressionRatio);

    //
    // Retrieve frame ID
    //
    // *** NOTES ***
    // Integers are returned as an int64_t.
    //
    int64_t frameID = 0;

    err = spinImageChunkDataGetIntValue(hImage, "ChunkFrameID", &frameID);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf(
            "Unable to retrieve frame ID from image chunk data. Aborting with error: %s [%d]\n\n",
            GetLastErrorMessage(),
            err);
        return err;
    }

    printf("\tFrame ID: %d\n", (int)frameID);

    // Retrieve gain; gain recorded in decibels
    double gain = 0.0;

    err = spinImageChunkDataGetFloatValue(hImage, "ChunkGain", &gain);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf(
            "Unable to retrieve gain from image chunk data. Aborting with error: %s [%d]\n\n",
            GetLastErrorMessage(),
            err);
        return err;
    }

    printf("\tGain: %f\n", gain);

    // Retrieve height; height recorded in pixels
    int64_t height = 0;

    err = spinImageChunkDataGetIntValue(hImage, "ChunkHeight", &height);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf(
            "Unable to retrieve height from image chunk data. Aborting with error: %s [%d]\n\n",
            GetLastErrorMessage(),
            err);
        return err;
    }

    printf("\tHeight: %d\n", (int)height);

    // Retrieve offset X; offset X recorded in pixels
    int64_t offsetX = 0;

    err = spinImageChunkDataGetIntValue(hImage, "ChunkOffsetX", &offsetX);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf(
            "Unable to retrieve offset X from image chunk data. Aborting with error: %s [%d]\n\n",
            GetLastErrorMessage(),
            err);
        return err;
    }

    printf("\tOffset X: %d\n", (int)offsetX);

    // Retrieve offset Y; offset Y recorded in pixels
    int64_t offsetY = 0;

    err = spinImageChunkDataGetIntValue(hImage, "ChunkOffsetY", &offsetY);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf(
            "Unable to retrieve offset Y from image chunk data. Aborting with error: %s [%d]\n\n",
            GetLastErrorMessage(),
            err);
        return err;
    }

    printf("\tOffset Y: %d\n", (int)offsetY);

    // Retrieve width; width recorded in pixels
    int64_t width = 0;

    err = spinImageChunkDataGetIntValue(hImage, "ChunkWidth", &width);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf(
            "Unable to retrieve width from image chunk data. Aborting with error: %s [%d]\n\n",
            GetLastErrorMessage(),
            err);
        return err;
    }

    printf("\tWidth: %d\n", (int)width);

    // Retrieve Serial Data
    int64_t serialDataLength = 0;

    err = spinImageChunkDataGetIntValue(hImage, "ChunkSerialDataLength", &serialDataLength);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf(
            "Unable to retrieve serial data length from image chunk data. Aborting with error: %s [%d]\n\n",
            GetLastErrorMessage(),
            err);
        return err;
    }

    if (serialDataLength > 0)
    {
        printf("\tSerial Data (%lld) Bytes:", serialDataLength);
        bool8_t serialReceiveOverFlow = 0;
        err = spinImageChunkDataGetBoolValue(hImage, "ChunkSerialReceiveOverflow", &serialReceiveOverFlow);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf(
                "Unable to retrieve serial receive overflow value from image chunk data. Aborting with error: %s "
                "[%d]\n\n",
                GetLastErrorMessage(),
                err);
            return err;
        }
        if (serialReceiveOverFlow)
        {
            printf("\t    Warning: Device serial data buffer overflow\n");
        }

        uint8_t* pSerialData = 0;

        err = spinImageChunkDataGetRawData(hImage, "ChunkSerialData", &pSerialData);
        if (err != SPINNAKER_ERR_SUCCESS || pSerialData == NULL)
        {
            if (pSerialData == NULL)
            {
                printf("Unable to retrieve serial data from image chunk data. Serial data was null\n\n");
            }
            printf(
                "Unable to retrieve serial data from image chunk data. Aborting with error: %s [%d]\n\n",
                GetLastErrorMessage(),
                err);
            return err;
        }
        printBytesAsHex(pSerialData, serialDataLength);
    }

    // Retrieve black level; black level recorded as a percentage
    double blackLevel = 0.0;
    err = spinImageChunkDataGetFloatValue(hImage, "ChunkBlackLevel", &blackLevel);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf(
            "Unable to retrieve black level from image chunk data. Aborting with error: %s [%d]\n\n",
            GetLastErrorMessage(),
            err);
        return err;
    }

    printf("\tBlack level: %f\n", blackLevel);

    return err;
}

// This function displays all available chunk data by looping through the chunk
// data category node on the nodemap.
spinError DisplayChunkDataFromNodeMap(spinNodeMapHandle hNodeMap)
{
    spinError err = SPINNAKER_ERR_SUCCESS;
    unsigned int i = 0;

    //
    // Retrieve chunk data information nodes
    //
    // *** NOTES ***
    // As well as being written into the payload of the image, chunk data is
    // accessible on the GenICam nodemap. Insofar as chunk data is
    // enabled, it is available from both sources.
    //
    spinNodeHandle hChunkDataControl = NULL;
    size_t numFeatures = 0;

    err = spinNodeMapGetNode(hNodeMap, "ChunkDataControl", &hChunkDataControl);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf("Unable to retrieve chunk data control. Aborting with error: %s [%d]\n\n", GetLastErrorMessage(), err);
        return err;
    }

    if (!IsReadable(hChunkDataControl, "ChunkDataControl"))
    {
        printf("Unable to retrieve chunk data control. Aborting...\n\n");
        return SPINNAKER_ERR_ERROR;
    }

    err = spinCategoryGetNumFeatures(hChunkDataControl, &numFeatures);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf("Unable to retrieve number of nodes (error %d)...\n\n", err);
        return err;
    }

    // Iterate through children
    printf("Printing chunk data from nodemap...\n");

    for (i = 0; i < numFeatures; i++)
    {
        spinNodeHandle hFeatureNode = NULL;
        spinNodeType featureType = UnknownNode;
        char featureName[MAX_BUFF_LEN];
        size_t lenFeatureName = MAX_BUFF_LEN;

        // Retrieve node
        if (IsReadable(hChunkDataControl, "ChunkDataControl"))
        {
            err = spinCategoryGetFeatureByIndex(hChunkDataControl, i, &hFeatureNode);
            if (err != SPINNAKER_ERR_SUCCESS)
            {
                printf("Unable to retrieve node (error %d)...\n\n", err);
                continue;
            }
        }
        else
        {
            err = SPINNAKER_ERR_ACCESS_DENIED;
            printf("Unable to retrieve node (error %d)...\n\n", err);
            continue;
        }

        // Retrieve node name
        err = spinNodeGetName(hFeatureNode, featureName, &lenFeatureName);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            strcpy(featureName, "Unknown name");
        }

        if (IsReadable(hFeatureNode, featureName))
        {
            err = spinNodeGetType(hFeatureNode, &featureType);
            if (err != SPINNAKER_ERR_SUCCESS)
            {
                printf("Unable to retrieve node type. Non-fatal error: %s [%d]\n\n", GetLastErrorMessage(), err);
                continue;
            }
        }
        else
        {
            err = SPINNAKER_ERR_ACCESS_DENIED;
            printf("Unable to retrieve node type. Non-fatal error: %s [%d]\n\n", GetLastErrorMessage(), err);
            continue;
        }

        // Print integer node type value
        if (featureType == IntegerNode)
        {
            int64_t featureValue = 0;

            err = spinIntegerGetValue(hFeatureNode, &featureValue);

            printf("\t%s: %d\n", featureName, (int)featureValue);
        }
        // Print float node type value
        else if (featureType == FloatNode)
        {
            double featureValue = 0.0;

            err = spinFloatGetValue(hFeatureNode, &featureValue);

            printf("\t%s: %f\n", featureName, featureValue);
        }
        //
        // Print boolean node type value
        //
        // *** NOTES ***
        // Boolean information is manipulated to output the more-easily
        // identifiable 'true' and 'false' as opposed to '1' and '0'.
        //
        else if (featureType == BooleanNode)
        {
            bool8_t featureValue = False;

            err = spinBooleanGetValue(hFeatureNode, &featureValue);

            if (featureValue)
            {
                printf("\t%s: true\n", featureName);
            }
            else
            {
                printf("\t%s: false\n", featureName);
            }
        }
    }

    return err;
}

// This function prints the device information of the camera from the transport
// layer; please see NodeMapInfo_C example for more in-depth comments on
// printing device information from the nodemap.
spinError PrintDeviceInfo(spinNodeMapHandle hNodeMap)
{
    spinError err = SPINNAKER_ERR_SUCCESS;
    unsigned int i = 0;

    printf("\n*** DEVICE INFORMATION ***\n\n");

    // Retrieve device information category node
    spinNodeHandle hDeviceInformation = NULL;

    err = spinNodeMapGetNode(hNodeMap, "DeviceInformation", &hDeviceInformation);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf("Unable to retrieve node. Non-fatal error: %s [%d]\n\n", GetLastErrorMessage(), err);
        return err;
    }

    // Retrieve number of nodes within device information node
    size_t numFeatures = 0;

    if (IsReadable(hDeviceInformation, "DeviceInformation"))
    {
        err = spinCategoryGetNumFeatures(hDeviceInformation, &numFeatures);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf("Unable to retrieve number of nodes. Non-fatal error: %s [%d]\n\n", GetLastErrorMessage(), err);
            return err;
        }
    }
    else
    {
        err = SPINNAKER_ERR_ACCESS_DENIED;
        printf("Unable to read device information. Non-fatal error: %s [%d]\n\n", GetLastErrorMessage(), err);
        return err;
    }

    // Iterate through nodes and print information
    for (i = 0; i < numFeatures; i++)
    {
        spinNodeHandle hFeatureNode = NULL;

        err = spinCategoryGetFeatureByIndex(hDeviceInformation, i, &hFeatureNode);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf("Unable to retrieve node. Non-fatal error: %s [%d]\n\n", GetLastErrorMessage(), err);
            continue;
        }

        spinNodeType featureType = UnknownNode;

        // Get feature node name
        char featureName[MAX_BUFF_LEN];
        size_t lenFeatureName = MAX_BUFF_LEN;

        err = spinNodeGetName(hFeatureNode, featureName, &lenFeatureName);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            strcpy(featureName, "Unknown name");
        }

        if (IsReadable(hFeatureNode, featureName))
        {
            err = spinNodeGetType(hFeatureNode, &featureType);
            if (err != SPINNAKER_ERR_SUCCESS)
            {
                printf("Unable to retrieve node type. Non-fatal error: %s [%d]\n\n", GetLastErrorMessage(), err);
                continue;
            }
        }
        else
        {
            printf("%s: Node not readable\n", featureName);
            continue;
        }

        char featureValue[MAX_BUFF_LEN];
        size_t lenFeatureValue = MAX_BUFF_LEN;

        err = spinNodeToString(hFeatureNode, featureValue, &lenFeatureValue);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            strcpy(featureValue, "Unknown value");
        }

        printf("%s: %s\n", featureName, featureValue);
    }
    printf("\n");

    return err;
}

// This function acquires and saves 10 images from a device; please see
// Acquisition_C example for more in-depth comments on the acquisition of
// images.
spinError AcquireImages(spinCamera hCam, spinNodeMapHandle hNodeMap, spinNodeMapHandle hNodeMapTLDevice)
{

    spinError err = SPINNAKER_ERR_SUCCESS;

    printf("\n*** IMAGE ACQUISITION ***\n\n");

    // Set acquisition mode to continuous
    spinNodeHandle hAcquisitionMode = NULL;
    spinNodeHandle hAcquisitionModeContinuous = NULL;
    int64_t acquisitionModeContinuous = 0;

    err = spinNodeMapGetNode(hNodeMap, "AcquisitionMode", &hAcquisitionMode);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf(
            "Unable to set acquisition mode to continuous (node retrieval). Aborting with error: %s [%d]\n\n",
            GetLastErrorMessage(),
            err);
        return err;
    }

    if (IsReadable(hAcquisitionMode, "AcquisitionMode"))
    {
        err = spinEnumerationGetEntryByName(hAcquisitionMode, "Continuous", &hAcquisitionModeContinuous);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf(
                "Unable to set acquisition mode to continuous (entry 'continuous' retrieval). Aborting with error "
                "%d...\n\n",
                err);
            return err;
        }
    }
    else
    {
        err = SPINNAKER_ERR_ACCESS_DENIED;
        printf("Unable to read acquisition mode. Aborting with error: %s [%d]\n\n", GetLastErrorMessage(), err);
        return err;
    }

    if (IsReadable(hAcquisitionModeContinuous, "AcquisitionModeContinuous"))
    {
        err = spinEnumerationEntryGetIntValue(hAcquisitionModeContinuous, &acquisitionModeContinuous);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf(
                "Unable to set acquisition mode to continuous (entry int value retrieval). Aborting with error "
                "%d...\n\n",
                err);
            return err;
        }
    }
    else
    {
        err = SPINNAKER_ERR_ACCESS_DENIED;
        printf(
            "Unable to read acquisition mode continuous. Aborting with error: %s [%d]\n\n", GetLastErrorMessage(), err);
        return err;
    }

    // set acquisition mode to continuous
    if (IsWritable(hAcquisitionMode, "AcquisitionMode"))
    {
        err = spinEnumerationSetIntValue(hAcquisitionMode, acquisitionModeContinuous);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf(
                "Unable to set acquisition mode to continuous (entry int value setting). Aborting with error %d...\n\n",
                err);
            return err;
        }
        printf("Acquisition mode set to continuous...\n");
    }
    else
    {
        err = SPINNAKER_ERR_ACCESS_DENIED;
        printf("Unable to write to acquisition mode. Aborting with error: %s [%d]\n\n", GetLastErrorMessage(), err);
        return err;
    }

    // Begin acquiring images
    err = spinCameraBeginAcquisition(hCam);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf("Unable to begin image acquisition. Aborting with error: %s [%d]\n\n", GetLastErrorMessage(), err);
        return err;
    }

    printf("Acquiring images...\n");

    // Retrieve device serial number for filename
    spinNodeHandle hDeviceSerialNumber = NULL;
    char deviceSerialNumber[MAX_BUFF_LEN];
    size_t lenDeviceSerialNumber = MAX_BUFF_LEN;

    err = spinNodeMapGetNode(hNodeMapTLDevice, "DeviceSerialNumber", &hDeviceSerialNumber);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        strcpy(deviceSerialNumber, "");
        lenDeviceSerialNumber = 0;
    }
    else
    {
        if (IsReadable(hDeviceSerialNumber, "DeviceSerialNumber"))
        {
            err = spinStringGetValue(hDeviceSerialNumber, deviceSerialNumber, &lenDeviceSerialNumber);
            if (err != SPINNAKER_ERR_SUCCESS)
            {
                printf("Unable to get device serial number. Non-fatal error: %s [%d]\n\n", GetLastErrorMessage(), err);
                strcpy(deviceSerialNumber, "");
                lenDeviceSerialNumber = 0;
            }

            printf("Device serial number retrieved as %s...\n", deviceSerialNumber);
        }
        else
        {
            err = SPINNAKER_ERR_ACCESS_DENIED;
            printf("Unable to get device serial number. Non-fatal error: %s [%d]\n\n", GetLastErrorMessage(), err);
            strcpy(deviceSerialNumber, "");
            lenDeviceSerialNumber = 0;
        }
    }
    printf("\n");

    // Retrieve, convert, and save images
    const unsigned int k_numImages = 10;
    unsigned int imageCnt = 0;

    //
    // Create Image Processor context for post processing images
    //
    spinImageProcessor hImageProcessor = NULL;
    err = spinImageProcessorCreate(&hImageProcessor);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf("Unable to create image processor. Non-fatal error: %s [%d]\n\n", GetLastErrorMessage(), err);
    }

    //
    // Set default image processor color processing method
    //
    // *** NOTES ***
    // By default, if no specific color processing algorithm is set, the image
    // processor will default to NEAREST_NEIGHBOR method.
    //
    err = spinImageProcessorSetColorProcessing(hImageProcessor, SPINNAKER_COLOR_PROCESSING_ALGORITHM_HQ_LINEAR);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf(
            "Unable to set image processor color processing method. Non-fatal error: %s [%d]\n\n",
            GetLastErrorMessage(),
            err);
    }

    for (imageCnt = 0; imageCnt < k_numImages; imageCnt++)
    {
        // Retrieve next received image
        spinImage hResultImage = NULL;

        err = spinCameraGetNextImageEx(hCam, 1000, &hResultImage);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf("Unable to get next image. Non-fatal error: %s [%d]\n\n", GetLastErrorMessage(), err);
            continue;
        }

        // Ensure image completion
        bool8_t isIncomplete = False;
        bool8_t hasFailed = False;

        err = spinImageIsIncomplete(hResultImage, &isIncomplete);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf("Unable to determine image completion. Non-fatal error: %s [%d]\n\n", GetLastErrorMessage(), err);
            hasFailed = True;
        }

        if (isIncomplete)
        {
            spinImageStatus imageStatus = SPINNAKER_IMAGE_STATUS_NO_ERROR;

            err = spinImageGetStatus(hResultImage, &imageStatus);
            if (err != SPINNAKER_ERR_SUCCESS)
            {
                printf("Unable to retrieve image status. Non-fatal error: %s [%d]\n\n", GetLastErrorMessage(), err);
            }
            else
            {
                printf("Image incomplete with image status %d...\n", imageStatus);
            }

            hasFailed = True;
        }

        // Release incomplete or failed image
        if (hasFailed)
        {
            err = spinImageRelease(hResultImage);
            if (err != SPINNAKER_ERR_SUCCESS)
            {
                printf("Unable to release image. Non-fatal error: %s [%d]\n\n", GetLastErrorMessage(), err);
            }

            continue;
        }

        // Print image information
        size_t width = 0;
        size_t height = 0;

        err = spinImageGetWidth(hResultImage, &width);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf("Unable to retrieve image width. Non-fatal error: %s [%d]\n\n", GetLastErrorMessage(), err);
        }

        err = spinImageGetHeight(hResultImage, &height);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf("Unable to retrieve image height. Non-fatal error: %s [%d]\n\n", GetLastErrorMessage(), err);
        }

        printf("Grabbed image %u, width = %u, height = %u\n", imageCnt, (unsigned int)width, (unsigned int)height);

        // Convert image to mono 8
        spinImage hConvertedImage = NULL;

        err = spinImageCreateEmpty(&hConvertedImage);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf("Unable to create image. Non-fatal error: %s [%d]\n\n", GetLastErrorMessage(), err);
            hasFailed = True;
        }

        err = spinImageProcessorConvert(hImageProcessor, hResultImage, hConvertedImage, PixelFormat_Mono8);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf("Unable to convert image. Non-fatal error: %s [%d]\n\n", GetLastErrorMessage(), err);
            hasFailed = True;
        }

        // Create unique file name and save image
        char filename[MAX_BUFF_LEN];

        if (lenDeviceSerialNumber == 0)
        {
            sprintf(filename, "ChunkData-C-%d.jpg", imageCnt);
        }
        else
        {
            sprintf(filename, "ChunkData-C-%s-%d.jpg", deviceSerialNumber, imageCnt);
        }

        err = spinImageSave(hConvertedImage, filename, SPINNAKER_IMAGE_FILE_FORMAT_JPEG);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf("Unable to save image. Non-fatal error: %s [%d]\n\n", GetLastErrorMessage(), err);
        }
        else
        {
            printf("Image saved at %s\n", filename);
        }

        // Display chunk data
        if (chosenChunkData == IMAGE)
        {
            err = DisplayChunkDataFromImage(hResultImage);
            if (err != SPINNAKER_ERR_SUCCESS)
            {
                return err;
            }
        }
        else if (chosenChunkData == NODEMAP)
        {
            err = DisplayChunkDataFromNodeMap(hNodeMap);
            if (err != SPINNAKER_ERR_SUCCESS)
            {
                return err;
            }
        }
        printf("\n");

        // Destroy converted image
        err = spinImageDestroy(hConvertedImage);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf("Unable to destroy image. Non-fatal error: %s [%d]\n\n", GetLastErrorMessage(), err);
        }

        // Release image
        err = spinImageRelease(hResultImage);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf("Unable to release image. Non-fatal error: %s [%d]\n\n", GetLastErrorMessage(), err);
        }
    }

    //
    // Destroy Image Processor context
    //
    // *** NOTES ***
    // Image processor context needs to be destroyed after all image processing
    // are complete to avoid memory leaks.
    //
    err = spinImageProcessorDestroy(hImageProcessor);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf("Unable to destroy image processor. Non-fatal error: %s [%d]\n\n", GetLastErrorMessage(), err);
    }

    // End Acquisition
    err = spinCameraEndAcquisition(hCam);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf("Unable to end acquisition. Non-fatal error: %s [%d]\n\n", GetLastErrorMessage(), err);
    }

    return err;
}

// This function disables each type of chunk data before disabling chunk data mode.
spinError DisableChunkData(spinNodeMapHandle hNodeMap)
{
    spinError err = SPINNAKER_ERR_SUCCESS;

    spinNodeHandle hChunkSelector = NULL;
    size_t numEntries = 0;
    unsigned int i = 0;

    // Retrieve selector node
    err = spinNodeMapGetNode(hNodeMap, "ChunkSelector", &hChunkSelector);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf(
            "Unable to retrieve chunk selector entries. Aborting with error: %s [%d]\n\n", GetLastErrorMessage(), err);
        return err;
    }

    // Retrieve number of entries, check if readable
    if (IsReadable(hChunkSelector, "ChunkSelector"))
    {
        err = spinEnumerationGetNumEntries(hChunkSelector, &numEntries);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf(
                "Unable to retrieve number of entries. Aborting with error: %s [%d]\n\n", GetLastErrorMessage(), err);
            return err;
        }
    }
    else
    {
        err = SPINNAKER_ERR_ACCESS_DENIED;
        printf("Unable to read number of entries. Aborting with error: %s [%d]\n\n", GetLastErrorMessage(), err);
        return err;
    }

    printf("Disabling entries...\n");

    for (i = 0; i < numEntries; i++)
    {
        // Retrieve entry node
        spinNodeHandle hEntry = NULL;

        err = spinEnumerationGetEntryByIndex(hChunkSelector, i, &hEntry);

        // Go to next node if problem occurs
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            continue;
        }

        // Retrieve entry name
        char entryName[MAX_BUFF_LEN];
        size_t lenEntryName = MAX_BUFF_LEN;

        if (IsReadable(hEntry, "ChunkEntry"))
        {
            err = spinNodeGetDisplayName(hEntry, entryName, &lenEntryName);
            if (err != SPINNAKER_ERR_SUCCESS)
            {
                printf("\t%s: unable to retrieve chunk entry by display name (error %d)...\n", entryName, err);
            }
        }
        else
        {
            continue;
        }
        // Retrieve enum entry integer value
        int64_t value = 0;

        err = spinEnumerationEntryGetIntValue(hEntry, &value);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf("\t%s: unable to get chunk entry value (error %d)...\n", entryName, err);
            continue;
        }

        // Set integer value
        if (IsWritable(hChunkSelector, "ChunkSelector"))
        {
            err = spinEnumerationSetIntValue(hChunkSelector, value);
            if (err != SPINNAKER_ERR_SUCCESS)
            {
                printf("\t%s: unable to set chunk entry value (error %d)...\n", entryName, err);
                continue;
            }
        }
        else
        {
            err = SPINNAKER_ERR_ACCESS_DENIED;
            printf("\t%s: unable to set chunk entry value (error %d)...\n", entryName, err);
            return err;
        }

        // Retrieve corresponding chunk enable node
        spinNodeHandle hChunkEnable = NULL;
        err = spinNodeMapGetNode(hNodeMap, "ChunkEnable", &hChunkEnable);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf("\t%s: unable to get entry from nodemap (error %d)...\n", entryName, err);
            continue;
        }

        // Retrieve chunk enable value and set to false if necessary
        bool8_t isEnabled = False;

        if (IsWritable(hChunkEnable, "ChunkEnable"))
        {
            err = spinBooleanGetValue(hChunkEnable, &isEnabled);
            if (err != SPINNAKER_ERR_SUCCESS)
            {
                printf("\t%s: unable to get chunk entry boolean value (error %d)...\n", entryName, err);
                continue;
            }
        }
        else
        {
            printf("\t%s: not writable\n", entryName);
            continue;
        }

        // Consider the case in which chunk data is enabled but not writable
        if (isEnabled)
        {
            // Set chunk enable value to false
            err = spinBooleanSetValue(hChunkEnable, False);
            if (err != SPINNAKER_ERR_SUCCESS)
            {
                printf("\t%s: unable to set chunk entry boolean value (error %d)...\n", entryName, err);
                continue;
            }
        }

        printf("\t%s: disabled\n", entryName);
    }

    printf("\n");

    // Disabling ChunkModeActive
    spinNodeHandle hChunkModeActive = NULL;

    err = spinNodeMapGetNode(hNodeMap, "ChunkModeActive", &hChunkModeActive);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf("Unable to get ChunkModeActive node. Aborting with error: %s [%d]\n\n", GetLastErrorMessage(), err);
        return err;
    }

    if (IsWritable(hChunkModeActive, "ChunkModeActive"))
    {
        err = spinBooleanSetValue(hChunkModeActive, False);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf("Unable to deactivate chunk mode. Aborting with error: %s [%d]\n\n", GetLastErrorMessage(), err);
            return err;
        }
    }
    else
    {
        err = SPINNAKER_ERR_ACCESS_DENIED;
        printf("Unable to write to chunk mode. Aborting with error: %s [%d]\n\n", GetLastErrorMessage(), err);
        return err;
    }

    printf("Chunk mode deactivated...\n");
    return err;
}

// This function acts as the body of the example; please see NodeMapInfo_C
// example for more in-depth comments on setting up cameras.
spinError RunSingleCamera(spinCamera hCam)
{
    spinError err = SPINNAKER_ERR_SUCCESS;

    // Retrieve TL device nodemap and print device information
    spinNodeMapHandle hNodeMapTLDevice = NULL;

    err = spinCameraGetTLDeviceNodeMap(hCam, &hNodeMapTLDevice);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf("Unable to retrieve TL device nodemap. Non-fatal error: %s [%d]\n\n", GetLastErrorMessage(), err);
    }
    else
    {
        err = PrintDeviceInfo(hNodeMapTLDevice);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            return err;
        }
    }

    // This example is not compatible with BX (Bumblebee) stereo cameras, see StereoAcquisiton (C++ only) for this camera.
    spinNodeHandle hDeviceModelName = NULL;
    char deviceModelName[MAX_BUFF_LEN] = {0};
    size_t modelNameLen = MAX_BUFF_LEN;
    spinNodeMapGetNode(hNodeMapTLDevice, "DeviceModelName", &hDeviceModelName);
    if (hDeviceModelName != NULL && IsReadable(hDeviceModelName, "DeviceModelName"))
    {
        err = spinStringGetValue(hDeviceModelName, deviceModelName, &modelNameLen);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            return err;
        }
        if (strstr(deviceModelName, "BX") != NULL)
        {
            printf("This example is not compatible with BX (Bumblebee) stereo cameras. Please see "
                   "StereoAcquisition (C++ Only) Example for ChunkData usage with this camera.\n");
            return SPINNAKER_ERR_ERROR;
        }
    }

    // Initialize camera
    err = spinCameraInit(hCam);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf("Unable to initialize camera. Aborting with error: %s [%d]\n\n", GetLastErrorMessage(), err);
        return err;
    }

    // Retrieve GenICam nodemap
    spinNodeMapHandle hNodeMap = NULL;

    err = spinCameraGetNodeMap(hCam, &hNodeMap);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf("Unable to retrieve GenICam nodemap. Aborting with error: %s [%d]\n\n", GetLastErrorMessage(), err);
        return err;
    }

    // Configure chunk data
    err = ConfigureChunkData(hNodeMap);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        return err;
    }

    // Acquire images and display chunk data
    err = AcquireImages(hCam, hNodeMap, hNodeMapTLDevice);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        return err;
    }

    // Disable chunck data
    err = DisableChunkData(hNodeMap);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        return err;
    }

    // Deinitialize camera
    err = spinCameraDeInit(hCam);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf("Unable to deinitialize camera. Non-fatal error: %s [%d]\n\n", GetLastErrorMessage(), err);
    }

    return err;
}

// Example entry point; please see Enumeration_C example for more in-depth
// comments on preparing and cleaning up the system.
int main(/*int argc, char** argv*/)
{
    spinError errReturn = SPINNAKER_ERR_SUCCESS;
    spinError err = SPINNAKER_ERR_SUCCESS;
    unsigned int i = 0;

    // Since this application saves images in the current folder
    // we must ensure that we have permission to write to this folder.
    // If we do not have permission, fail right away.
    FILE* tempFile;
    tempFile = fopen("test.txt", "w+");
    if (tempFile == NULL)
    {
        printf("Failed to create file in current folder.  Please check "
               "permissions.\n");
        printf("Press Enter to exit...\n");
        getchar();
        return -1;
    }
    fclose(tempFile);
    remove("test.txt");

    // Print application build information
    printf("Application build date: %s %s \n\n", __DATE__, __TIME__);

    // Retrieve singleton reference to system
    spinSystem hSystem = NULL;

    err = spinSystemGetInstance(&hSystem);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf("Unable to retrieve system instance. Aborting with error: %s [%d]\n\n", GetLastErrorMessage(), err);
        return err;
    }

    // Print out current library version
    spinLibraryVersion hLibraryVersion;

    spinSystemGetLibraryVersion(hSystem, &hLibraryVersion);
    printf(
        "Spinnaker library version: %d.%d.%d.%d\n\n",
        hLibraryVersion.major,
        hLibraryVersion.minor,
        hLibraryVersion.type,
        hLibraryVersion.build);

    // Retrieve list of cameras from the system
    spinCameraList hCameraList = NULL;

    err = spinCameraListCreateEmpty(&hCameraList);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf("Unable to create camera list. Aborting with error: %s [%d]\n\n", GetLastErrorMessage(), err);
        return err;
    }

    err = spinSystemGetCameras(hSystem, hCameraList);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf("Unable to retrieve camera list. Aborting with error: %s [%d]\n\n", GetLastErrorMessage(), err);
        return err;
    }

    // Retrieve number of cameras
    size_t numCameras = 0;

    err = spinCameraListGetSize(hCameraList, &numCameras);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf("Unable to retrieve number of cameras. Aborting with error: %s [%d]\n\n", GetLastErrorMessage(), err);
        return err;
    }

    printf("Number of cameras detected: %u\n\n", (unsigned int)numCameras);

    // Finish if there are no cameras
    if (numCameras == 0)
    {
        // Clear and destroy camera list before releasing system
        err = spinCameraListClear(hCameraList);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf("Unable to clear camera list. Aborting with error: %s [%d]\n\n", GetLastErrorMessage(), err);
            return err;
        }

        err = spinCameraListDestroy(hCameraList);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf("Unable to destroy camera list. Aborting with error: %s [%d]\n\n", GetLastErrorMessage(), err);
            return err;
        }

        // Release system
        err = spinSystemReleaseInstance(hSystem);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf("Unable to release system instance. Aborting with error: %s [%d]\n\n", GetLastErrorMessage(), err);
            return err;
        }

        printf("Not enough cameras!\n");
        printf("Done! Press Enter to exit...\n");
        getchar();

        return -1;
    }

    // Run example on each camera
    for (i = 0; i < numCameras; i++)
    {
        printf("\nRunning example for camera %d...\n", i);

        // Select camera
        spinCamera hCamera = NULL;

        err = spinCameraListGet(hCameraList, i, &hCamera);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf("Unable to retrieve camera from list. Aborting with error: %s [%d]\n\n", GetLastErrorMessage(), err);
            errReturn = err;
        }
        else
        {
            // Run example
            err = RunSingleCamera(hCamera);
            if (err != SPINNAKER_ERR_SUCCESS)
            {
                errReturn = err;
            }
        }

        // Release camera
        err = spinCameraRelease(hCamera);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            errReturn = err;
        }

        printf("Camera %d example complete...\n\n", i);
    }

    // Clear and destroy camera list before releasing system
    err = spinCameraListClear(hCameraList);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf("Unable to clear camera list. Aborting with error: %s [%d]\n\n", GetLastErrorMessage(), err);
        return err;
    }

    err = spinCameraListDestroy(hCameraList);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf("Unable to destroy camera list. Aborting with error: %s [%d]\n\n", GetLastErrorMessage(), err);
        return err;
    }

    // Release system
    err = spinSystemReleaseInstance(hSystem);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf("Unable to release system instance. Aborting with error: %s [%d]\n\n", GetLastErrorMessage(), err);
        return err;
    }

    printf("\nDone! Press Enter to exit...\n");
    getchar();

    return errReturn;
}
