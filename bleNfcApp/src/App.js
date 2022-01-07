/**
 * Sample React Native App
 * https://github.com/facebook/react-native
 *
 * @format
 * @flow strict-local
 */

import React, {useState, useEffect} from 'react';
import {
  FlatList,
  KeyboardAvoidingView,
  NativeEventEmitter,
  NativeModules,
  SafeAreaView,
  ScrollView,
  StyleSheet,
  Text,
  TextInput,
  TouchableOpacity,
  View,
} from 'react-native';
import {NavigationContainer} from '@react-navigation/native';
import {createNativeStackNavigator} from '@react-navigation/native-stack';
import AwsFreertos, {
  Characteristic,
  eventKeys,
} from 'react-native-aws-freertos';

import {LogBox} from 'react-native';
LogBox.ignoreLogs(['new NativeEventEmitter']); // Ignore log notification by message
// LogBox.ignoreAllLogs(); //Ignore all log notifications

const Stack = createNativeStackNavigator();

// Function to handle BLE scanning and connecting
const bluetoothScreen = ({navigation}) => {
  const [result, setResult] = useState([]);
  const [scanning, setScanning] = useState(false);
  const [connectingToDevice, setConnectingToDevice] = useState(null);

  useEffect(() => {
    try {
      // Always request BT and location permissions from smartphone
      AwsFreertos.requestBtPermissions();
      const eventEmitter = new NativeEventEmitter(NativeModules.AwsFreertos);
      // Create an array of important events and relevant actions to be taken if they occur
      const btEvents = [];
      btEvents.push(
        eventEmitter.addListener(eventKeys.DID_DISCOVERED_DEVICE, device => {
          if (Array.isArray(device)) {
            setResult([...result, ...device]);
          } else {
            if (result.some(r => device.macAddr === r.macAddr)) return;
            setResult([...result, device]);
          }
        }),
      );
      btEvents.push(
        eventEmitter.addListener(eventKeys.DID_DISCONNECT_DEVICE, device => {
          alert('Device disconnected. \n Try again.');
        }),
      );
      btEvents.push(
        eventEmitter.addListener(eventKeys.DID_CONNECT_DEVICE, device => {
          console.log('Connected');
          setConnectingToDevice(false);
          setTimeout(() => {
            navigation.navigate('Send Credentials', {
              deviceMacAddress: device.macAddr,
              deviceName: device.name,
            });
          }, 1000);
        }),
      );
      btEvents.push(
        eventEmitter.addListener(
          eventKeys.DID_FAIL_TO_CONNECT_DEVICE,
          device => {
            console.warn(
              'Failed to connect to device - MAC address: ' + device.macAddr,
            );
          },
        ),
      );
      return () => {
        AwsFreertos.stopScanBtDevices();
        btEvents.forEach(btEvent => btEvent.remove());
      };
    } catch (error) {
      console.warn(error);
    }
  }, []);

  // BLE scan timeout in ms
  let scanTimeout = null;

  const onScanBtDevices = () => {
    if (scanTimeout !== null) {
      clearTimeout(scanTimeout);
    }
    console.log('Scanning...');
    setScanning(true);
    scanTimeout = setInterval(() => setScanning(false), 10000);
    AwsFreertos.startScanBtDevices();
  };
  const onConnectToDevice = device => () => {
    AwsFreertos.connectDevice(device.macAddr);
    console.log('MAC address: ' + device.macAddr);
  };

  // Format the devices in the flatlist
  const ListItem = ({item, onPress, backgroundColor, textColor}) => (
    <TouchableOpacity onPress={onPress} style={[styles.item, backgroundColor]}>
      <Text style={[styles.nameText, textColor]}>Name: {item.name}</Text>
      <Text style={[styles.subText, textColor]}>
        MAC address/UUID: {item.macAddr}
      </Text>
    </TouchableOpacity>
  );

  const renderListItem = ({item}) => {
    const backgroundColor = item.name === result.name ? 'peru' : 'papayawhip';
    const color = item.name === result.name ? 'white' : 'grey';

    return (
      <ListItem
        item={item}
        onPress={onConnectToDevice(item)}
        backgroundColor={{backgroundColor}}
        textColor={{color}}
      />
    );
  };

  return (
    <SafeAreaView style={styles.container}>
      <TouchableOpacity style={styles.button} onPress={onScanBtDevices}>
        <Text style={{color: 'snow', fontSize: 18}}>
          {'Scan for BLE Devices'}
        </Text>
      </TouchableOpacity>
      <FlatList data={Object.values(result)} renderItem={renderListItem} />
    </SafeAreaView>
  );
};

// Scan and list nearby WiFi networks
const wifiScan = ({route, navigation}) => {
  const uniqBy = (a, compareFun) => {
    return a.reduce((accumulator, currentValue) => {
      if (!accumulator.some(item => compareFun(item, currentValue))) {
        return [...accumulator, currentValue];
      }
      return accumulator;
    }, []);
  };

  const [isScanningDeviceWifiNetworks, setIsScanningDeviceWifiNetworks] =
    useState(false);
  const [selectedNetwork, setSelectedNetwork] = useState(null);
  const [errorSavingNetwork, setErrorSavingNetwork] = useState(false);
  const [availableWifiNetworks, setAvailableWifiNetworks] = useState([]);
  const [networkToDelete, setNetworkToDelete] = useState(null);
  const {deviceMacAddress} = route.params;
  const wifiAvailableScannedNetworks = [];

  useEffect(() => {
    try {
      let interval = setInterval(() => {
        AwsFreertos.getConnectedDeviceAvailableNetworks(deviceMacAddress);
      }, 2000);
      setIsScanningDeviceWifiNetworks(true);
      const eventEmitter = new NativeEventEmitter(NativeModules.AwsFreertos);
      const wifiEvents = [];
      wifiEvents.push(
        eventEmitter.addListener(eventKeys.DID_LIST_NETWORK, network => {
          console.log(network);
          if (Array.isArray(network))
            network.forEach(net => wifiAvailableScannedNetworks.push(net));
          else wifiAvailableScannedNetworks.push(network);
        }),
      );
      wifiEvents.push(
        eventEmitter.addListener(eventKeys.DID_SAVE_NETWORK, wifi => {
          setErrorSavingNetwork(false);
          AwsFreertos.disconnectDevice(deviceMacAddress);
          navigation.navigate('Success', {
            deviceMacAddress,
            wifiSsid: wifi && wifi.ssid,
          });
        }),
      );
      wifiEvents.push(
        eventEmitter.addListener(eventKeys.ERROR_SAVE_NETWORK, () => {
          setErrorSavingNetwork(true);
        }),
      );
      wifiEvents.push(
        eventEmitter.addListener(eventKeys.DID_DELETE_NETWORK, () => {
          setNetworkToDelete(null);
          setAvailableWifiNetworks([]);
          AwsFreertos.getConnectedDeviceAvailableNetworks(deviceMacAddress);
        }),
      );
      const wifiInterval = setInterval(() => {
        setAvailableWifiNetworks(
          uniqBy(wifiAvailableScannedNetworks, (a, b) => a.bssid === b.bssid),
        );
      }, 2000);

      return () => {
        clearInterval(interval);
        clearInterval(wifiInterval);
        wifiEvents.forEach(event => event.remove());
      };
    } catch (error) {
      console.warn(error);
    }
  }, []);

  return (
    <SafeAreaView style={styles.container}>
      {errorSavingNetwork && <Text>Error saving network</Text>}
      {isScanningDeviceWifiNetworks && (
        <Text style={{fontSize: 18, paddingTop: 20}}>
          Scanning for Wi-Fi networks...
        </Text>
      )}
      {availableWifiNetworks && availableWifiNetworks.length > 0 && (
        <ScrollView
          style={{
            borderBottomWidth: 1,
            backgroundColor: 'snow',
            textColor: 'grey',
            fontSize: 16,
          }}>
          {availableWifiNetworks.map((network, k) => (
            <View key={network.bssid}>
              <View key={network.bssid} style={styles.networkTextContainer}>
                <TouchableOpacity
                  style={{
                    justifyContent: 'center',
                    borderRadius: 4,
                    backgroundColor: 'papayawhip',
                    height: 50,
                    width: 300,
                  }}
                  onPress={() => {
                    navigation.navigate('Send Password', {
                      deviceMacAddress,
                      selectedBssid: network.bssid,
                      selectedSsid: network.ssid,
                      isConnected: network.connected,
                    });
                  }}>
                  <Text style={styles.subText}>{network.ssid}</Text>
                </TouchableOpacity>
              </View>
            </View>
          ))}
        </ScrollView>
      )}
    </SafeAreaView>
  );
};

// Transfer credentials to ESP32
const wifiProvision = ({route, navigation}) => {
  const [pwValue, setPwValue] = useState(null);
  const {deviceMacAddress, selectedBssid, selectedSsid, isConnected} =
    route.params;
  console.log('Params:' + JSON.stringify(route.params));
  console.log('Mac Address: ' + deviceMacAddress);
  console.log('Wi-Fi SSID: ' + selectedSsid);
  console.log('Wi-Fi BSSID: ' + selectedBssid);

  const onConnectToNetwork = networkBsid => () => {
    AwsFreertos.saveNetworkOnConnectedDevice(
      deviceMacAddress,
      networkBsid,
      pwValue,
    );
  };

  // Do not need to implement this as yet.
  // This function deletes a Wi-Fi network that was saved on the ESP32
  // const disconnectFromNetwork = network => {
  //   if (networkToDelete) return;
  //   setNetworkToDelete(network);
  //   AwsFreertos.disconnectNetworkOnConnectedDevice(
  //     deviceMacAddress,
  //     network.index,
  //   );
  // };

  return (
    <KeyboardAvoidingView
      style={styles.container}
      behavior="height"
      keyboardVerticalOffset={-110}>
      <Text style={{fontSize: 18, padding: 10}}>
        Send Wi-Fi Credentials to Selected Device
      </Text>
      <View
        style={{
          backgroundColor: 'tan',
          padding: 10,
          borderRadius: 4,
          height: '25%',
          width: '90%',
          alignItems: 'center',
          justifyContent: 'center',
        }}>
        <View
          style={{
            backgroundColor: 'wheat',
            padding: 5,
            borderRadius: 4,
            height: '40%',
            width: '100%',
            justifyContent: 'center',
          }}>
          <View
            style={{
              flexDirection: 'row',
              justifyContent: 'center',
            }}>
            <Text style={(styles.subText, {paddingVertical: 10})}>
              Wi-Fi SSID:
            </Text>
            <Text style={(styles.subText, styles.input)}>{selectedSsid}</Text>
          </View>
        </View>

        <View
          style={{
            marginTop: 20,
            padding: 5,
            backgroundColor: 'wheat',
            borderRadius: 4,
            height: '40%',
            width: '100%',
            justifyContent: 'center',
          }}>
          <View
            style={{
              flexDirection: 'row',
              justifyContent: 'center',
            }}>
            <Text style={(styles.subText, {paddingVertical: 10})}>
              Password:
            </Text>
            <TextInput
              style={(styles.subText, styles.input)}
              placeholder="Enter password here"
              onChangeText={val => setPwValue(val)}
              defaultValue={pwValue}
              value={pwValue}
            />
          </View>
        </View>
      </View>

      <TouchableOpacity
        style={styles.button}
        onPress={onConnectToNetwork(selectedBssid)}>
        <Text style={{color: 'snow', fontSize: 18}}>{'Send'}</Text>
      </TouchableOpacity>
    </KeyboardAvoidingView>
  );
};

const successScreen = ({route, navigation}) => {
  const {deviceMacAddress, wifiSsid} = route.params;
  return (
    <View style={styles.container}>
      <View
        style={
          (styles.button, {alignItems: 'center', backgroundColor: 'snow'})
        }>
        <Text style={styles.nameText}>Wi-Fi provisioning success!</Text>
        <Text style={styles.nameText}>
          The ESP32 with MAC address {deviceMacAddress} is now connected to{' '}
          {wifiSsid}
        </Text>
      </View>
    </View>
  );
};

function App() {
  return (
    <NavigationContainer>
      <Stack.Navigator>
        <Stack.Screen
          name="Connect Bluetooth"
          component={bluetoothScreen}
          options={{
            title: 'Wi-Fi Connect for ESP32',
            headerStyle: {
              backgroundColor: 'cadetblue',
            },
            headerTintColor: 'snow',
            headerTintStyle: {
              fontWeight: 'bold',
            },
          }}
        />
        <Stack.Screen
          name="Send Credentials"
          component={wifiScan}
          options={{
            title: 'Wi-Fi Connect for ESP32',
            headerStyle: {
              backgroundColor: 'cadetblue',
            },
            headerTintColor: 'snow',
            headerTintStyle: {
              fontWeight: 'bold',
            },
          }}
        />
        <Stack.Screen
          name="Send Password"
          component={wifiProvision}
          options={{
            title: 'Wi-Fi Connect for ESP32',
            headerStyle: {
              backgroundColor: 'cadetblue',
            },
            headerTintColor: 'snow',
            headerTintStyle: {
              fontWeight: 'bold',
            },
          }}
        />
        <Stack.Screen
          name="Success"
          component={successScreen}
          options={{
            title: 'Wi-Fi Connect for ESP32',
            headerStyle: {
              backgroundColor: 'cadetblue',
            },
            headerTintColor: 'snow',
            headerTintStyle: {
              fontWeight: 'bold',
            },
          }}
        />
      </Stack.Navigator>
    </NavigationContainer>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: 'snow',
    alignItems: 'center',
  },
  button: {
    marginTop: 50,
    marginBottom: 20,
    paddingVertical: 10,
    paddingHorizontal: 30,
    borderRadius: 4,
    backgroundColor: 'tan',
    alignItems: 'center',
    minWidth: '48%',
  },
  item: {
    padding: 20,
    marginVertical: 8,
    marginHorizontal: 16,
    borderRadius: 4,
  },
  nameText: {
    fontSize: 16,
  },
  subText: {
    fontSize: 14,
  },
  input: {
    backgroundColor: 'papayawhip',
    borderRadius: 4,
    width: '75%',
    justifyContent: 'center',
    padding: 10,
  },
  networkTextContainer: {
    paddingVertical: 16,
    display: 'flex',
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
});

export default App;
