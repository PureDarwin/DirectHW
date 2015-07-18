/* DirectHW - Kernel extension to pass through IO commands to user space
 *
 * Copyright © 2008-2010 coresystems GmbH <info@coresystems.de>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <IOKit/IOLib.h>
#include <IOKit/IOService.h>
#include <IOKit/IOUserClient.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/IOMemoryDescriptor.h>
#include <architecture/i386/pio.h>

#include "DirectHW.hpp"

#undef DEBUG_KEXT
//#define DEBUG_KEXT

#define super IOService

OSDefineMetaClassAndStructors(DirectHWService, IOService)

bool DirectHWService::start(IOService * provider)
{
        //IOLog("DirectHW: Driver v%s (compiled on %s) loaded. "
		//"Visit http://www.coresystems.de/ for more information.\n",
			//DIRECTHW_VERSION, __DATE__);

	if (super::start(provider)) {
		registerService();
		return true;
	}

	return false;
}

#undef super
#define super IOUserClient

OSDefineMetaClassAndStructors(DirectHWUserClient, IOUserClient)

const IOExternalMethod DirectHWUserClient::fMethods[kNumberOfMethods] = {
	{0, (IOMethod) & DirectHWUserClient::ReadIO, kIOUCStructIStructO, sizeof(iomem_t), sizeof(iomem_t)},
	{0, (IOMethod) & DirectHWUserClient::WriteIO, kIOUCStructIStructO, sizeof(iomem_t), sizeof(iomem_t)},
	{0, (IOMethod) & DirectHWUserClient::PrepareMap, kIOUCStructIStructO, sizeof(map_t), sizeof(map_t)},
	{0, (IOMethod) & DirectHWUserClient::ReadMSR, kIOUCStructIStructO, sizeof(msrcmd_t), sizeof(msrcmd_t)},
	{0, (IOMethod) & DirectHWUserClient::WriteMSR, kIOUCStructIStructO, sizeof(msrcmd_t), sizeof(msrcmd_t)}
};

bool DirectHWUserClient::initWithTask(task_t task, void *securityID, UInt32 type)
{
	bool ret;

	ret = super::initWithTask(task, securityID, type);

#ifdef DEBUG_KEXT
	IOLog("DirectHW: initWithTask(%p, %p, %08lx)\n", (void *)task, securityID, type);
#endif
        if (!ret) {
		IOLog("DirectHW: initWithTask failed.\n");
		return false;
	}

	fTask = task;

	return true;
}

IOExternalMethod *DirectHWUserClient::getTargetAndMethodForIndex(IOService ** target, UInt32 index)
{
	if (index < (UInt32) kNumberOfMethods) {
		if (fMethods[index].object == (IOService *) 0)
			*target = this;

		return (IOExternalMethod *) & fMethods[index];
	} else {
		*target = NULL;
		return NULL;
	}
}

bool DirectHWUserClient::start(IOService * provider)
{
	bool success;

#ifdef DEBUG_KEXT
	IOLog("DirectHW: Starting DirectHWUserClient\n");
#endif

	fProvider = OSDynamicCast(DirectHWService, provider);
	success = (fProvider != NULL);
/*
	if (kIOReturnSuccess != clientHasPrivilege(current_task(),kIOClientPrivilegeAdministrator)) {
		IOLog("DirectHW: Need to be administrator.\n");
		success = false;
	}
*/
	if (success) {
		success = super::start(provider);
#ifdef DEBUG_KEXT
		IOLog("DirectHW: Client successfully started.\n");
#endif
	} else {
		IOLog("DirectHW: Could not start client.\n");
	}
	return success;
}

void DirectHWUserClient::stop(IOService *provider)
{
#ifdef DEBUG_KEXT
	IOLog("DirectHW: Stopping client.\n");
#endif
	super::stop(provider);
}

IOReturn DirectHWUserClient::clientClose(void)
{
	bool success = terminate();
	if (!success) {
		IOLog("DirectHW: Client NOT successfully closed.\n");
	} else {
#ifdef DEBUG_KEXT
		IOLog("DirectHW: Client successfully closed.\n");
#endif
	}

	return kIOReturnSuccess;
}

IOReturn
DirectHWUserClient::ReadIO(iomem_t * inStruct, iomem_t * outStruct,
				IOByteCount inStructSize,
				IOByteCount * outStructSize)
{

	if (fProvider == NULL || isInactive()) {
		return kIOReturnNotAttached;
	}
	 
	switch (inStruct->width) {
	case 1:
		outStruct->data = inb(inStruct->offset);
		break;
	case 2:
		outStruct->data = inw(inStruct->offset);
		break;
	case 4:
		outStruct->data = inl(inStruct->offset);
		break;
	default:
		IOLog("DirectHW: Invalid read attempt %d bytes at IO address %x\n",
		      (int)inStruct->width, (unsigned int)inStruct->offset);
		break;
	}

#ifdef DEBUG_KEXT
	IOLog("DirectHW: Read %d bytes at IO address %x (result=%x)\n",
		      inStruct->width, inStruct->offset, outStruct->data);
#endif


	*outStructSize = sizeof(iomem_t);

	return kIOReturnSuccess;
}


IOReturn
DirectHWUserClient::WriteIO(iomem_t * inStruct, iomem_t * outStruct,
				 IOByteCount inStructSize,
				 IOByteCount * outStructSize)
{
	if (fProvider == NULL || isInactive()) {
		return kIOReturnNotAttached;
	} 
	
#ifdef DEBUG_KEXT
	IOLog("DirectHW: Write %d bytes at IO address %x (value=%x)\n",
		      inStruct->width, inStruct->offset, inStruct->data);
#endif
	
	switch (inStruct->width) {
	case 1:
		outb(inStruct->offset, inStruct->data);
		break;
	case 2:
		outw(inStruct->offset, inStruct->data);
		break;
	case 4:
		outl(inStruct->offset, inStruct->data);
		break;
	default:
		IOLog("DirectHW: Invalid write attempt %d bytes at IO address %x\n",
		      (int)inStruct->width, (unsigned int)inStruct->offset);
	}

	*outStructSize = sizeof(iomem_t);

	return kIOReturnSuccess;
}


IOReturn
DirectHWUserClient::PrepareMap(map_t * inStruct, map_t * outStruct,
				IOByteCount inStructSize,
				IOByteCount * outStructSize)
{
	if (fProvider == NULL || isInactive()) {
		return kIOReturnNotAttached;
	} 

	if(LastMapAddr || LastMapSize)
		return kIOReturnNotOpen;

	LastMapAddr = inStruct->addr;
	LastMapSize = inStruct->size;

#ifdef DEBUG_KEXT
	IOLog("DirectHW: PrepareMap 0x%08x[0x%x]\n", LastMapAddr, LastMapSize);
#endif

	*outStructSize = sizeof(map_t);

	return kIOReturnSuccess;
}

inline void
DirectHWUserClient::cpuid(uint32_t op1, uint32_t op2, uint32_t *data)
{
	asm("cpuid"
		: "=a" (data[0]),
		"=b" (data[1]),
		"=c" (data[2]),
		"=d" (data[3])
		: "a"(op1), "c"(op2));
}

void
DirectHWUserClient::MSRHelperFunction(void *data)
{
	MSRHelper * MSRData = (MSRHelper *)data;
	msrcmd_t * inStruct = MSRData->in;
	msrcmd_t * outStruct = MSRData->out;

	outStruct->core = -1;
	outStruct->lo = INVALID_MSR_LO;
	outStruct->hi = INVALID_MSR_HI;

	uint32_t cpuiddata[4];

	cpuid(1, 0, cpuiddata);
	bool have_ht = ((cpuiddata[3] & (1 << 28)) != 0);
	uint32_t core_id = cpuiddata[1] >> 24;

	cpuid(11, 0, cpuiddata);
	uint32_t smt_mask = ~((-1) << (cpuiddata[0] &0x1f));

	// TODO: What we want is this:
	// if (inStruct->core != cpu_to_core(cpu_number()))
	//	return;

	if ((core_id & smt_mask) != core_id)
		return; // It's a HT thread

	if (inStruct->core != cpu_number())
		return;

	IOLog("DirectHW: ReadMSRHelper %d %d %x \n", inStruct->core,
		cpu_number(), smt_mask);

	if (MSRData->Read) {
		asm volatile (
			"rdmsr"
			: "=a" (outStruct->lo), "=d" (outStruct->hi)
			: "c" (inStruct->index)
		);
	} else {
		asm volatile (
			"wrmsr"
			: /* No outputs */
			: "c" (inStruct->index), "a" (inStruct->lo), "d" (inStruct->hi)
		);
	}

	outStruct->index = inStruct->index;
	outStruct->core = inStruct->core;
}

IOReturn
DirectHWUserClient::ReadMSR(msrcmd_t * inStruct, msrcmd_t * outStruct,
				IOByteCount inStructSize,
				IOByteCount * outStructSize)
{
	if (fProvider == NULL || isInactive()) {
		return kIOReturnNotAttached;
	} 

	MSRHelper MSRData = { inStruct, outStruct, true };
	mp_rendezvous(NULL, (void (*)(void *))MSRHelperFunction, NULL,
			(void *)&MSRData);

	*outStructSize = sizeof(msrcmd_t);

	if (outStruct->core != inStruct->core)
		return kIOReturnIOError;

	IOLog("DirectHW: ReadMSR(0x%08x) = 0x%08x:0x%08x\n",
			(unsigned int)inStruct->index,
			(unsigned int)outStruct->hi,
			(unsigned int)outStruct->lo);

	return kIOReturnSuccess;
}

IOReturn
DirectHWUserClient::WriteMSR(msrcmd_t * inStruct, msrcmd_t * outStruct,
				IOByteCount inStructSize,
				IOByteCount * outStructSize)
{
	if (fProvider == NULL || isInactive()) {
		return kIOReturnNotAttached;
	} 

	IOLog("DirectHW: WriteMSR(0x%08x) = 0x%08x:0x%08x\n",
			(unsigned int)inStruct->index,
			(unsigned int)inStruct->hi,
			(unsigned int)inStruct->lo);

	MSRHelper MSRData = { inStruct, outStruct, false };
	mp_rendezvous(NULL, (void (*)(void *))MSRHelperFunction, NULL,
			(void *)&MSRData);

	*outStructSize = sizeof(msrcmd_t);

	if (outStruct->core != inStruct->core)
		return kIOReturnIOError;

	return kIOReturnSuccess;
}

IOReturn DirectHWUserClient::clientMemoryForType(UInt32 type, UInt32 *flags, IOMemoryDescriptor **memory)
{
	IOMemoryDescriptor *newmemory;

#ifdef DEBUG_KEXT
	IOLog("DirectHW: clientMemoryForType(%x, %p, %p)\n", type, flags, memory);
#endif
	if (type != 0) {
		IOLog("DirectHW: Unknown mapping type %x.\n", (unsigned int)type);
		return kIOReturnUnsupported;
	}

	if ((LastMapAddr == 0) && (LastMapSize == 0)) {
		IOLog("DirectHW: No PrepareMap called.\n");
		return kIOReturnNotAttached;
	}

#ifdef DEBUG_KEXT
	IOLog("DirectHW: Mapping physical 0x%08x[0x%x]\n",
			LastMapAddr, LastMapSize);
#endif

	newmemory = IOMemoryDescriptor::withPhysicalAddress(LastMapAddr, LastMapSize, kIODirectionIn);
	
	/* Reset mapping to zero */
	LastMapAddr = 0;
	LastMapSize = 0;

	if (newmemory == 0) {
		IOLog("DirectHW: Could not map memory!\n");
		return kIOReturnNotOpen;
	}

	newmemory->retain();
	*memory = newmemory;

#ifdef DEBUG_KEXT
	IOLog("DirectHW: Mapping succeeded.\n");
#endif

	return kIOReturnSuccess;
}

