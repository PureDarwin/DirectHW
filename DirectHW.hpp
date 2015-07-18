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

#define DIRECTHW_VERSION "1.3"
#define DIRECTHW_VERNUM 0x00100300

/* */

class DirectHWService:public IOService {
	OSDeclareDefaultStructors(DirectHWService)

      public:
	virtual bool start(IOService * provider);

};


/* */

class DirectHWService;

class DirectHWUserClient:public IOUserClient {
	OSDeclareDefaultStructors(DirectHWUserClient)

	enum {
		kReadIO,
		kWriteIO,
		kPrepareMap,
		kReadMSR,
		kWriteMSR,
		kNumberOfMethods
	};

	typedef struct {
		UInt32 offset;
		UInt32 width;
		UInt32 data;
	} iomem_t;

	typedef struct {
		UInt64 addr;
		UInt64 size;
	} map_t;

	typedef struct {
		UInt32 core;
		UInt32 index;
		UInt32 hi;
		UInt32 lo;
	} msrcmd_t;

      public:
	virtual bool initWithTask(task_t task, void *securityID, UInt32 type);

	virtual bool start(IOService * provider);
	virtual void stop(IOService * provider);

	virtual IOReturn clientMemoryForType(UInt32 type, UInt32 *flags, IOMemoryDescriptor **memory);

	virtual IOReturn clientClose(void);

      protected:
	DirectHWService * fProvider;

	static const IOExternalMethod fMethods[kNumberOfMethods];

	virtual IOExternalMethod * getTargetAndMethodForIndex(IOService ** target, UInt32 index);

	virtual IOReturn ReadIO(iomem_t * inStruct, iomem_t * outStruct,
				 IOByteCount inStructSize,
				 IOByteCount * outStructSize);

	virtual IOReturn WriteIO(iomem_t * inStruct, iomem_t * outStruct,
				  IOByteCount inStructSize,
				  IOByteCount * outStructSize);

	virtual IOReturn PrepareMap(map_t * inStruct, map_t * outStruct,
				  IOByteCount inStructSize,
				  IOByteCount * outStructSize);

	virtual IOReturn ReadMSR(msrcmd_t * inStruct, msrcmd_t * outStruct,
				  IOByteCount inStructSize,
				  IOByteCount * outStructSize);

	virtual IOReturn WriteMSR(msrcmd_t * inStruct, msrcmd_t * outStruct,
				  IOByteCount inStructSize,
				  IOByteCount * outStructSize);

      private:
	task_t fTask;
	UInt64 LastMapAddr, LastMapSize;

	static void MSRHelperFunction(void *data);
	typedef struct { msrcmd_t *in, *out; bool Read; } MSRHelper;
	static inline void cpuid(uint32_t op1, uint32_t op2, uint32_t *data);
};

extern "C" {

/* from sys/osfmk/i386/mp.c */

extern void mp_rendezvous(
	void (*setup_func)(void *),
	void (*action_func)(void *),
	void (*teardown_func)(void *),
	void *arg);

extern int cpu_number(void);

}

#define INVALID_MSR_LO 0x63744857
#define INVALID_MSR_HI 0x44697265
