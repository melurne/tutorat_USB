#include <libusb-1.0/libusb.h>
#include <stdlib.h>

#define NB_INTERFACES_MAX 8
#define TARGET_DEVICE_ID_VENDOR 0x03eb
#define TARGET_DEVICE_ID_PRODUCT 0x2043
#define MAX_DATA 32
#define NB_ENDPOINT_MAX 8

struct usb_device {
	libusb_device_handle *handle;
	int claimed_interfaces[NB_INTERFACES_MAX];
	int nb_claimed_interfaces;
	unsigned char endpoint[NB_ENDPOINT_MAX];
	int nb_saved_endpoints;
};

int close_device(struct usb_device *device) {
	for (int i = 0; i< device->nb_claimed_interfaces; i++) {
		libusb_release_interface(device->handle, device->claimed_interfaces[i]);
	}
	libusb_close(device->handle);
	device->nb_claimed_interfaces = 0;
	device->handle = NULL;
	return 0;
}

int main() {
	struct usb_device manette_struct;
	manette_struct.handle = NULL;
	manette_struct.nb_claimed_interfaces = 0;
	manette_struct.nb_saved_endpoints = 0;
	libusb_device *manette = NULL;
	libusb_context *context;
	int status=libusb_init(&context);
	if(status!=0) {perror("libusb_init"); exit(-1);}
	
	libusb_device **list;
	ssize_t count=libusb_get_device_list(context,&list);
	if(count<0) {perror("libusb_get_device_list"); exit(-1);}
		ssize_t i=0;
	for(i=0;i<count;i++){
  		libusb_device *device=list[i];
  		struct libusb_device_descriptor desc;
  		int status=libusb_get_device_descriptor(device,&desc);
  		if(status!=0) continue;
  		uint8_t bus=libusb_get_bus_number(device);
  		uint8_t address=libusb_get_device_address(device);
		if (desc.idVendor == TARGET_DEVICE_ID_VENDOR && desc.idProduct == TARGET_DEVICE_ID_PRODUCT) manette = device;
  		//printf("Device Found @ (Bus:Address) %d:%d\n",bus,address);
  		//printf("Vendor ID 0x0%x\n",desc.idVendor);
  		//printf("Product ID 0x0%x\n",desc.idProduct);
  	}

	libusb_free_device_list(list,1); 
	
	if (manette == NULL) exit(1);

	libusb_device_handle *handle;
	int status2=libusb_open(manette, &handle);
	if(status2!=0){ perror("libusb_open"); exit(-1); }
	
	manette_struct.handle = handle;
	struct libusb_config_descriptor *config;
	libusb_get_active_config_descriptor(manette, &config);

	printf("bNumInterfaces: %d, \n", config->bNumInterfaces);

	for (int i=0; i<config->bNumInterfaces; i++) {
		struct libusb_interface_descriptor *interface = config->interface[i].altsetting;
		printf("Interface %d : ", interface->bInterfaceNumber);
		if (interface->bInterfaceSubClass == LIBUSB_CLASS_VENDOR_SPEC) {
			printf("Claimed ");
		}
		printf("\n");
		int statusClaimed = libusb_claim_interface(manette_struct.handle, interface->bInterfaceNumber);
		if(statusClaimed!=0) {perror("claim error");} 
		
		manette_struct.claimed_interfaces[manette_struct.nb_claimed_interfaces] = interface->bInterfaceNumber;
		manette_struct.nb_claimed_interfaces++;
		for (int j=0; j<interface->bNumEndpoints; j++) {
			struct libusb_endpoint_descriptor endpoint = interface->endpoint[j];
			printf("\t Endpoint %d ", endpoint.bEndpointAddress);
			if (endpoint.bmAttributes & 0x03 == LIBUSB_ENDPOINT_TRANSFER_TYPE_INTERRUPT) {
				manette_struct.endpoint[manette_struct.nb_saved_endpoints] = endpoint.bEndpointAddress;
				manette_struct.nb_saved_endpoints++;
				printf("saved");	
			}
			printf("\n");
		} 
	}

	printf("%d, %d\n", manette_struct.endpoint[1], manette_struct.endpoint[2]);

	unsigned char endpoint_1=manette_struct.endpoint[1];    /* ID of endpoint (bit 8 is 1) */
	char data = 0xFF;    /* data to send or to receive */
	int size=1;           /* size to send or maximum size to receive */
	int timeout=1000;        /* timeout in ms */
	/* IN interrupt, host polling device */
	int bytes_in=0;

	int status3 = 1;
	status3=libusb_interrupt_transfer(manette_struct.handle,endpoint_1,&data,size,&bytes_in,timeout);
	printf("%d\n", bytes_in);
	if(status3!=0){ perror("libusb_interrupt_transfer"); exit(-1); }
	
	int endpoint_2=manette_struct.endpoint[2];    /* ID of endpoint (bit 8 is 1) */
	data = 0xFF;    /* data to send or to receive */
	size=1;           /* size to send or maximum size toInterrupt transfer.  receive */
	timeout=1000;        /* timeout in ms */
	/* IN interrupt, host polling device */
	bytes_in=0;

	status3 = 1;
	status3=libusb_interrupt_transfer(manette_struct.handle,endpoint_2,&data,size,NULL,timeout);
	if(status3!=0){ perror("libusb_interrupt_transfer"); exit(-1); }

	close_device(&manette_struct);
	libusb_exit(context);

	return 0;
}
