// BluetoothDiscovery.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <iostream>
#include <WinSock2.h>
#include <ws2bth.h>
#include <bluetoothapis.h>
#include <initguid.h>

#pragma comment(lib, "Bthprops.lib")
#pragma comment(lib, "Ws2_32.lib")

using namespace std;

// {25B12D43-557B-4060-95B3-96F98F66C04F}
DEFINE_GUID(ShareItServiceClass_UUID, 0x25b12d43, 0x557b, 0x4060, 0x95, 0xb3, 0x96, 0xf9, 0x8f, 0x66, 0xc0, 0x4f);

BOOL GetBluetoothRadio(PHANDLE phRadio)
{
	BOOL fResult(FALSE);

	DWORD cbComputerNameSize = MAX_COMPUTERNAME_LENGTH + 1;
	TCHAR pszComputerName[MAX_COMPUTERNAME_LENGTH + 1];
	ZeroMemory(pszComputerName, cbComputerNameSize);
	fResult = GetComputerName(pszComputerName, &cbComputerNameSize);

	BLUETOOTH_FIND_RADIO_PARAMS params;
	params.dwSize = sizeof(BLUETOOTH_FIND_RADIO_PARAMS);

	HBLUETOOTH_RADIO_FIND hRadioFind = NULL;
	HANDLE hRadio = NULL;
	hRadioFind = BluetoothFindFirstRadio(&params, &hRadio);

	if (hRadioFind == NULL)
	{
		goto ERROR_LABEL;
	}

	while (true)
	{
		DWORD dwError(0);
		BLUETOOTH_RADIO_INFO radioInfo;
		radioInfo.dwSize = sizeof(BLUETOOTH_RADIO_INFO);

		dwError = BluetoothGetRadioInfo(hRadio, &radioInfo);

		if (dwError == ERROR_SUCCESS)
		{
			if (lstrcmpi(pszComputerName, radioInfo.szName) == 0)
			{
				*phRadio = hRadio;
				fResult = TRUE;

				goto ERROR_LABEL;
			}
			else
			{
				// TODO: TEST WITH OTHER BLUETOOTH DEVICES WHICH HAVE IMPLEMENTED MICROSOFT BLUETOOTH STACK
				fResult = BluetoothFindNextRadio(hRadioFind, &hRadio);

				if (!fResult)
				{
					DWORD dwError(0);
					dwError = GetLastError();
					fResult = dwError == ERROR_NO_MORE_ITEMS ? TRUE : FALSE;

					goto ERROR_LABEL;
				}
			}
		}
	}

ERROR_LABEL:
	if (hRadioFind)
	{
		BluetoothFindRadioClose(hRadioFind);
	}

	return fResult;
}

BOOL TryEnableBluetoothDiscovery(HANDLE hRadio)
{
	BOOL fResult(FALSE);
	fResult = BluetoothIsDiscoverable(hRadio);

	if (!fResult)
	{
		BluetoothEnableDiscovery(hRadio, TRUE);
		fResult = BluetoothIsDiscoverable(hRadio);
	}

	return fResult;
}

BOOL AdvertiseService()
{
	BOOL fError(FALSE);
	GUID serviceID = ShareItServiceClass_UUID;
	WSADATA wsadata;

	BAIL_ON_ERROR(WSAStartup(MAKEWORD(2, 2), &wsadata))

	SOCKET s = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);

	if (s == INVALID_SOCKET)
	{
		return FALSE;
	}

	WSAPROTOCOL_INFO protocolInfo;
	INT protocolInfoSize = sizeof(protocolInfo);

	BAIL_ON_ERROR(getsockopt(
		s,
		SOL_SOCKET,
		SO_PROTOCOL_INFO,
		reinterpret_cast<PCHAR>(&protocolInfo),
		&protocolInfoSize))

	SOCKADDR_BTH address;
	address.addressFamily = AF_BTH;
	address.btAddr = 0;
	address.serviceClassId = GUID_NULL;
	address.port = BT_PORT_ANY;
	LPSOCKADDR lpSockAddr = reinterpret_cast<LPSOCKADDR>(&address);

	BAIL_ON_ERROR(bind(s, lpSockAddr, sizeof(SOCKADDR_BTH)))
	BAIL_ON_ERROR(listen(s, SOMAXCONN))

	WSAQUERYSET service;
	ZeroMemory(&service, sizeof(service));
	service.dwSize = sizeof(service);
	service.lpszServiceInstanceName = _T("ShareIt X.0");
	service.lpszComment = _T("Something useful could go here");
	service.lpServiceClassId = &serviceID;
	service.dwNumberOfCsAddrs = 1;
	service.dwNameSpace = NS_BTH;

	CSADDR_INFO csAddrInfo;
	ZeroMemory(&csAddrInfo, sizeof(csAddrInfo));
	csAddrInfo.LocalAddr.iSockaddrLength = sizeof(SOCKADDR_BTH);
	csAddrInfo.LocalAddr.lpSockaddr = lpSockAddr;
	csAddrInfo.iSocketType = SOCK_STREAM;
	csAddrInfo.iProtocol = BTHPROTO_RFCOMM;
	service.lpcsaBuffer = &csAddrInfo;

	BAIL_ON_ERROR(WSASetService(&service, RNRSERVICE_REGISTER, 0))

	_tprintf_s(_T("Service Advertisement succeeded.\n"));
	system("pause");

	WSASetService(&service, RNRSERVICE_DELETE, 0);
	fError = TRUE;

ERROR_LABEL:
	closesocket(s);
	WSACleanup();
	return fError;
}

VOID RunServer()
{
	_tprintf_s(_T("Check if Bluetooth radio is on: "));

	BOOL fError(FALSE);
	HANDLE hRadio(NULL);
	fError = GetBluetoothRadio(&hRadio);
	_tprintf_s(_T("%s\n"), fError ? _T("True") : _T("False"));

	if (!fError)
	{
		_tprintf_s(_T("Bluetooth Radio is either missing or turned off. Exiting...\n"));
		return;
	}

	_tprintf_s(_T("Check if Bluetooth discovery is on: "));

	fError = TryEnableBluetoothDiscovery(hRadio);
	_tprintf_s(_T("%s\n"), fError ? _T("True") : _T("False"));

	if (!fError)
	{
		_tprintf_s(_T("Bluetooth Discovery could not be turned on. Exiting...\n"));
		return;
	}

	AdvertiseService();
}

BOOL EnumerateServices()
{
	return FALSE;
}

BOOL EnumerateDevices()
{
	return FALSE;
}

BOOL DiscoverService()
{
	BOOL fError(FALSE);
	GUID serviceID = ShareItServiceClass_UUID;
	WSADATA wsadata;

	BAIL_ON_ERROR(WSAStartup(MAKEWORD(2, 2), &wsadata));

	SOCKET s = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);

	if (s == INVALID_SOCKET)
	{
		return FALSE;
	}

	WSAPROTOCOL_INFO protocolInfo;
	INT protocolInfoSize = sizeof(protocolInfo);

	BAIL_ON_ERROR(getsockopt(
		s,
		SOL_SOCKET,
		SO_PROTOCOL_INFO,
		reinterpret_cast<PCHAR>(&protocolInfo),
		&protocolInfoSize));

	WSAQUERYSET queryDeviceSet;
	ZeroMemory(&queryDeviceSet, sizeof(queryDeviceSet));
	queryDeviceSet.dwSize = sizeof(queryDeviceSet);
	queryDeviceSet.dwNameSpace = NS_BTH;
	DWORD dwFlags = LUP_CONTAINERS | LUP_FLUSHCACHE | LUP_RETURN_ADDR | LUP_RETURN_BLOB | LUP_RETURN_NAME | LUP_RETURN_TYPE;
	HANDLE hDeviceLookup(NULL);

	BAIL_ON_ERROR(WSALookupServiceBegin(&queryDeviceSet, dwFlags, &hDeviceLookup));

	dwFlags = LUP_CONTAINERS | LUP_RETURN_NAME | LUP_RETURN_ADDR | LUP_FLUSHCACHE;
	while (true)
	{
		BYTE deviceBuffer[1024];
		ZeroMemory(&deviceBuffer, sizeof(deviceBuffer));
		DWORD cbDeviceBufferLength(1024);
		LPWSAQUERYSET lpqsDeviceResults = reinterpret_cast<LPWSAQUERYSET>(&deviceBuffer);
		INT iError = WSALookupServiceNext(hDeviceLookup, dwFlags, &cbDeviceBufferLength, lpqsDeviceResults);

		if (iError != ERROR_SUCCESS)
		{
			iError = WSAGetLastError();

			if (iError == WSA_E_NO_MORE)
			{
				break;
			}
			continue;
		}

		_tprintf_s(_T("Device Name: %s\n"), lpqsDeviceResults->lpszServiceInstanceName);
		_tprintf_s(_T("--------------------------Begin----------------------------------\n"));

		WSAQUERYSET queryServiceSet;
		ZeroMemory(&queryServiceSet, sizeof(queryServiceSet));
		queryServiceSet.dwSize = sizeof(queryServiceSet);
		GUID protocol = L2CAP_PROTOCOL_UUID;
		queryServiceSet.lpServiceClassId = &protocol;
		queryServiceSet.dwNameSpace = NS_BTH;

		PCSADDR_INFO pCSAddrInfo = reinterpret_cast<PCSADDR_INFO>(lpqsDeviceResults->lpcsaBuffer);
		WCHAR szAddress[2048];
		ZeroMemory(szAddress, 2048);
		DWORD cbAddressSize = sizeof(szAddress);

		iError = WSAAddressToString(
			pCSAddrInfo->RemoteAddr.lpSockaddr,
			pCSAddrInfo->RemoteAddr.iSockaddrLength,
			&protocolInfo,
			szAddress,
			&cbAddressSize);

		if (iError != ERROR_SUCCESS)
		{
			continue;
		}

		queryServiceSet.lpszContext = szAddress;
		DWORD dwServiceFlags = LUP_FLUSHCACHE | LUP_RETURN_NAME | LUP_RETURN_COMMENT;

		HANDLE hSerivceLookup(NULL);
		iError = WSALookupServiceBegin(&queryServiceSet, dwServiceFlags, &hSerivceLookup);

		while (iError == 0)
		{
			BYTE serviceBuffer[2048];
			ZeroMemory(&serviceBuffer, sizeof(serviceBuffer));
			DWORD cbServiceBufferLength(2048);
			LPWSAQUERYSET lpqsServiceResults = reinterpret_cast<LPWSAQUERYSET>(&serviceBuffer);

			iError = WSALookupServiceNext(hSerivceLookup, dwServiceFlags, &cbServiceBufferLength, lpqsServiceResults);

			if (iError != ERROR_SUCCESS)
			{
				iError = WSAGetLastError();

				if (iError == WSA_E_NO_MORE)
				{
					break;
				}
				continue;
			}

			_tprintf_s(_T("Service Name: %s, Service Comment: %s\n"), lpqsServiceResults->lpszServiceInstanceName, lpqsServiceResults->lpszComment);
		}

		_tprintf_s(_T("--------------------------End----------------------------------\n"));
		WSALookupServiceEnd(hSerivceLookup);
	}
	WSALookupServiceEnd(hDeviceLookup);

ERROR_LABEL:
	closesocket(s);
	WSACleanup();
	return fError;
}

VOID RunClient()
{
	DiscoverService();
}

int _tmain(int argc, _TCHAR* argv[])
{
	int selection(0);
	
	while (true)
	{
		_tprintf_s(_T("Select Mode: 1. Server, 2. Client, 0. Exit\n"));
		_tscanf_s(_T("%d"), &selection);

		switch (selection)
		{
		case 1:
			RunServer();
			break;
		case 2:
			RunClient();
			break;
		case 0:
			return 0;
		}
	}

	return 0;
}

