/* The MIT License (MIT)
 *
 * Copyright (c) 2014-2018 David Medina and Tim Warburton
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 */

#include <map>

#include <occa/base.hpp>
#include <occa/memory.hpp>
#include <occa/device.hpp>
#include <occa/mode/serial/memory.hpp>
#include <occa/uva.hpp>
#include <occa/tools/sys.hpp>

namespace occa {
  //---[ modeMemory_t ]---------------------
  modeMemory_t::modeMemory_t(const occa::properties &properties_) {
    memInfo = uvaFlag::none;
    properties = properties_;

    ptr    = NULL;
    uvaPtr = NULL;

    modeDevice = NULL;

    size = 0;
    canBeFreed = true;
  }

  modeMemory_t::~modeMemory_t() {}

  bool modeMemory_t::isManaged() const {
    return (memInfo & uvaFlag::isManaged);
  }

  bool modeMemory_t::inDevice() const {
    return (memInfo & uvaFlag::inDevice);
  }

  bool modeMemory_t::isStale() const {
    return (memInfo & uvaFlag::isStale);
  }

  //---[ memory ]-----------------------
  memory::memory() :
    modeMemory(NULL) {}

  memory::memory(void *uvaPtr) :
    modeMemory(NULL) {
    ptrRangeMap::iterator it = uvaMap.find(uvaPtr);
    if (it != uvaMap.end()) {
      setModeMemory(it->second);
    } else {
      setModeMemory((modeMemory_t*) uvaPtr);
    }
  }

  memory::memory(modeMemory_t *modeMemory_) :
    modeMemory(NULL) {
    setModeMemory(modeMemory_);
  }

  memory::memory(const memory &m) :
    modeMemory(NULL) {
    setModeMemory(m.modeMemory);
  }

  memory& memory::operator = (const memory &m) {
    setModeMemory(m.modeMemory);
    return *this;
  }

  memory::~memory() {
    removeRef();
  }

  void memory::setModeMemory(modeMemory_t *modeMemory_) {
    if (modeMemory != modeMemory_) {
      removeRef();
      modeMemory = modeMemory_;
      modeMemory->addRef();
    }
  }

  void memory::setModeDevice(modeDevice_t *modeDevice) {
    modeMemory->modeDevice = modeDevice;
    // If this is the very first reference, update the device references
    if (modeMemory->getRefs() == 1) {
      modeMemory->modeDevice->addRef();
    }
  }

  void memory::removeRef() {
    if (modeMemory && !modeMemory->removeRef()) {
      free();
    }
  }

  void memory::dontUseRefs() {
    if (modeMemory) {
      modeMemory->dontUseRefs();
    }
  }

  bool memory::isInitialized() const {
    return (modeMemory != NULL);
  }

  memory& memory::swap(memory &m) {
    modeMemory_t *modeMemory_ = modeMemory;
    modeMemory   = m.modeMemory;
    m.modeMemory = modeMemory_;
    return *this;
  }

  void* memory::ptr() {
    return (modeMemory ? modeMemory->ptr : NULL);
  }

  const void* memory::ptr() const {
    return (modeMemory ? modeMemory->ptr : NULL);
  }

  modeMemory_t* memory::getModeMemory() const {
    return modeMemory;
  }

  modeDevice_t* memory::getModeDevice() const {
    return modeMemory->modeDevice;
  }

  occa::device memory::getDevice() const {
    return occa::device(modeMemory
                        ? modeMemory->modeDevice
                        : NULL);
  }

  memory::operator kernelArg() const {
    if (!modeMemory) {
      return kernelArg((void*) NULL);
    }
    return modeMemory->makeKernelArg();
  }

  const std::string& memory::mode() const {
    static const std::string noMode = "No Mode";
    return (modeMemory
            ? modeMemory->modeDevice->mode
            : noMode);
  }

  const occa::properties& memory::properties() const {
    static const occa::properties noProperties;
    return (modeMemory
            ? modeMemory->properties
            : noProperties);
  }

  udim_t memory::size() const {
    if (modeMemory == NULL) {
      return 0;
    }
    return modeMemory->size;
  }

  bool memory::isManaged() const {
    return (modeMemory && modeMemory->isManaged());
  }

  bool memory::inDevice() const {
    return (modeMemory && modeMemory->inDevice());
  }

  bool memory::isStale() const {
    return (modeMemory && modeMemory->isStale());
  }

  void memory::setupUva() {
    if (!modeMemory) {
      return;
    }
    if ( !(modeMemory->modeDevice->hasSeparateMemorySpace()) ) {
      modeMemory->uvaPtr = modeMemory->ptr;
    } else {
      modeMemory->uvaPtr = (char*) sys::malloc(modeMemory->size);
    }

    ptrRange range;
    range.start = modeMemory->uvaPtr;
    range.end   = (range.start + modeMemory->size);

    uvaMap[range] = modeMemory;
    modeMemory->modeDevice->uvaMap[range] = modeMemory;

    // Needed for kernelArg.void_ -> modeMemory checks
    if (modeMemory->uvaPtr != modeMemory->ptr) {
      uvaMap[modeMemory->ptr] = modeMemory;
    }
  }

  void memory::startManaging() {
    if (modeMemory) {
      modeMemory->memInfo |= uvaFlag::isManaged;
    }
  }

  void memory::stopManaging() {
    if (modeMemory) {
      modeMemory->memInfo &= ~uvaFlag::isManaged;
    }
  }

  void memory::syncToDevice(const dim_t bytes,
                            const dim_t offset) {
    OCCA_ERROR("Memory not initialized",
               modeMemory != NULL);

    udim_t bytes_ = ((bytes == -1) ? modeMemory->size : bytes);

    OCCA_ERROR("Trying to copy negative bytes (" << bytes << ")",
               bytes >= -1);
    OCCA_ERROR("Cannot have a negative offset (" << offset << ")",
               offset >= 0);

    if (bytes_ == 0) {
      return;
    }

    OCCA_ERROR("Memory has size [" << modeMemory->size << "],"
               << " trying to access [" << offset << ", " << (offset + bytes_) << "]",
               (bytes_ + offset) <= modeMemory->size);

    if (!modeMemory->modeDevice->hasSeparateMemorySpace()) {
      return;
    }

    copyFrom(modeMemory->uvaPtr, bytes_, offset);

    modeMemory->memInfo |=  uvaFlag::inDevice;
    modeMemory->memInfo &= ~uvaFlag::isStale;

    removeFromStaleMap(modeMemory);
  }

  void memory::syncToHost(const dim_t bytes,
                          const dim_t offset) {
    OCCA_ERROR("Memory not initialized",
               modeMemory != NULL);

    udim_t bytes_ = ((bytes == -1) ? modeMemory->size : bytes);

    OCCA_ERROR("Trying to copy negative bytes (" << bytes << ")",
               bytes >= -1);
    OCCA_ERROR("Cannot have a negative offset (" << offset << ")",
               offset >= 0);

    if (bytes_ == 0) {
      return;
    }

    OCCA_ERROR("Memory has size [" << modeMemory->size << "],"
               << " trying to access [" << offset << ", " << (offset + bytes_) << "]",
               (bytes_ + offset) <= modeMemory->size);

    if (!modeMemory->modeDevice->hasSeparateMemorySpace()) {
      return;
    }

    copyTo(modeMemory->uvaPtr, bytes_, offset);

    modeMemory->memInfo &= ~uvaFlag::inDevice;
    modeMemory->memInfo &= ~uvaFlag::isStale;

    removeFromStaleMap(modeMemory);
  }

  bool memory::uvaIsStale() const {
    return (modeMemory && modeMemory->isStale());
  }

  void memory::uvaMarkStale() {
    if (modeMemory != NULL) {
      modeMemory->memInfo |= uvaFlag::isStale;
    }
  }

  void memory::uvaMarkFresh() {
    if (modeMemory != NULL) {
      modeMemory->memInfo &= ~uvaFlag::isStale;
    }
  }

  bool memory::operator == (const occa::memory &m) const {
    return (modeMemory == m.modeMemory);
  }

  bool memory::operator != (const occa::memory &m) const {
    return (modeMemory != m.modeMemory);
  }

  occa::memory memory::operator + (const dim_t offset) const {
    return slice(offset);
  }

  occa::memory& memory::operator += (const dim_t offset) {
    *this = slice(offset);
    return *this;
  }

  occa::memory memory::slice(const dim_t offset,
                             const dim_t bytes) const {
    OCCA_ERROR("Memory not initialized",
               modeMemory != NULL);

    udim_t bytes_ = ((bytes == -1)
                     ? (modeMemory->size - offset)
                     : bytes);

    OCCA_ERROR("Trying to allocate negative bytes (" << bytes << ")",
               bytes >= -1);

    OCCA_ERROR("Cannot have a negative offset (" << offset << ")",
               offset >= 0);

    OCCA_ERROR("Cannot have offset and bytes greater than the memory size ("
               << offset << " + " << bytes_ << " > " << size() << ")",
               (offset + (dim_t) bytes_) <= (dim_t) size());

    occa::memory m(modeMemory->addOffset(offset));
    modeMemory_t &mv = *(m.modeMemory);
    mv.modeDevice = modeMemory->modeDevice;
    mv.size = bytes_;
    if (modeMemory->uvaPtr) {
      mv.uvaPtr = (modeMemory->uvaPtr + offset);
    }
    return m;
  }

  void memory::copyFrom(const void *src,
                        const dim_t bytes,
                        const dim_t offset,
                        const occa::properties &props) {
    OCCA_ERROR("Memory not initialized",
               modeMemory != NULL);

    udim_t bytes_ = ((bytes == -1) ? modeMemory->size : bytes);

    OCCA_ERROR("Trying to allocate negative bytes (" << bytes << ")",
               bytes >= -1);

    OCCA_ERROR("Cannot have a negative offset (" << offset << ")",
               offset >= 0);

    OCCA_ERROR("Destination memory has size [" << modeMemory->size << "],"
               << "trying to access [" << offset << ", " << (offset + bytes_) << "]",
               (bytes_ + offset) <= modeMemory->size);

    modeMemory->copyFrom(src, bytes_, offset, props);
  }

  void memory::copyFrom(const memory src,
                        const dim_t bytes,
                        const dim_t destOffset,
                        const dim_t srcOffset,
                        const occa::properties &props) {
    OCCA_ERROR("Memory not initialized",
               modeMemory && src.modeMemory);

    udim_t bytes_ = ((bytes == -1) ? modeMemory->size : bytes);

    OCCA_ERROR("Trying to allocate negative bytes (" << bytes << ")",
               bytes >= -1);

    OCCA_ERROR("Cannot have a negative offset (" << destOffset << ")",
               destOffset >= 0);

    OCCA_ERROR("Cannot have a negative offset (" << srcOffset << ")",
               srcOffset >= 0);

    OCCA_ERROR("Source memory has size [" << src.modeMemory->size << "],"
               << "trying to access [" << srcOffset << ", " << (srcOffset + bytes_) << "]",
               (bytes_ + srcOffset) <= src.modeMemory->size);

    OCCA_ERROR("Destination memory has size [" << modeMemory->size << "],"
               << "trying to access [" << destOffset << ", " << (destOffset + bytes_) << "]",
               (bytes_ + destOffset) <= modeMemory->size);

    modeMemory->copyFrom(src.modeMemory, bytes_, destOffset, srcOffset, props);
  }

  void memory::copyTo(void *dest,
                      const dim_t bytes,
                      const dim_t offset,
                      const occa::properties &props) const {
    OCCA_ERROR("Memory not initialized",
               modeMemory != NULL);

    udim_t bytes_ = ((bytes == -1) ? modeMemory->size : bytes);

    OCCA_ERROR("Trying to allocate negative bytes (" << bytes << ")",
               bytes >= -1);

    OCCA_ERROR("Cannot have a negative offset (" << offset << ")",
               offset >= 0);

    OCCA_ERROR("Source memory has size [" << modeMemory->size << "],"
               << "trying to access [" << offset << ", " << (offset + bytes_) << "]",
               (bytes_ + offset) <= modeMemory->size);

    modeMemory->copyTo(dest, bytes_, offset, props);
  }

  void memory::copyTo(memory dest,
                      const dim_t bytes,
                      const dim_t destOffset,
                      const dim_t srcOffset,
                      const occa::properties &props) const {
    OCCA_ERROR("Memory not initialized",
               modeMemory && dest.modeMemory);

    udim_t bytes_ = ((bytes == -1) ? modeMemory->size : bytes);

    OCCA_ERROR("Trying to allocate negative bytes (" << bytes << ")",
               bytes >= -1);

    OCCA_ERROR("Cannot have a negative offset (" << destOffset << ")",
               destOffset >= 0);

    OCCA_ERROR("Cannot have a negative offset (" << srcOffset << ")",
               srcOffset >= 0);

    OCCA_ERROR("Source memory has size [" << modeMemory->size << "],"
               << "trying to access [" << srcOffset << ", " << (srcOffset + bytes_) << "]",
               (bytes_ + srcOffset) <= modeMemory->size);

    OCCA_ERROR("Destination memory has size [" << dest.modeMemory->size << "],"
               << "trying to access [" << destOffset << ", " << (destOffset + bytes_) << "]",
               (bytes_ + destOffset) <= dest.modeMemory->size);

    dest.modeMemory->copyFrom(modeMemory, bytes_, destOffset, srcOffset, props);
  }

  void memory::copyFrom(const void *src,
                        const occa::properties &props) {
    copyFrom(src, -1, 0, props);
  }

  void memory::copyFrom(const memory src,
                        const occa::properties &props) {
    copyFrom(src, -1, 0, 0, props);
  }

  void memory::copyTo(void *dest,
                      const occa::properties &props) const {
    copyTo(dest, -1, 0, props);
  }

  void memory::copyTo(const memory dest,
                      const occa::properties &props) const {
    copyTo(dest, -1, 0, 0, props);
  }

  occa::memory memory::clone() const {
    if (modeMemory) {
      return occa::device(modeMemory->modeDevice).malloc(size(),
                                                         *this,
                                                         properties());
    }
    return occa::memory();
  }

  void memory::free() {
    deleteRefs(true);
  }

  void memory::detach() {
    deleteRefs(false);
  }

  void memory::deleteRefs(const bool freeMemory) {
    if (modeMemory == NULL) {
      return;
    }
    if (!modeMemory->canBeFreed) {
      delete modeMemory;
      modeMemory = NULL;
      return;
    }

    modeDevice_t *modeDevice = modeMemory->modeDevice;
    modeDevice->bytesAllocated -= (modeMemory->size);

    if (modeMemory->uvaPtr) {
      uvaMap.erase(modeMemory->uvaPtr);
      modeDevice->uvaMap.erase(modeMemory->uvaPtr);

      // CPU case where memory is shared
      if (modeMemory->uvaPtr != modeMemory->ptr) {
        uvaMap.erase(modeMemory->ptr);
        modeDevice->uvaMap.erase(modeMemory->uvaPtr);

        sys::free(modeMemory->uvaPtr);
      }
    }

    if (freeMemory) {
      modeMemory->free();
    } else {
      modeMemory->detach();
    }

    modeDevice->removeRef();
    delete modeMemory;
    modeMemory = NULL;
  }

  std::ostream& operator << (std::ostream &out,
                             const occa::memory &memory) {
    out << memory.properties();
    return out;
  }

  namespace cpu {
    occa::memory wrapMemory(void *ptr, const udim_t bytes) {
      serial::memory &mem = *(new serial::memory);
      mem.dontUseRefs();

      mem.modeDevice = host().getModeDevice();
      mem.size = bytes;
      mem.ptr = (char*) ptr;

      return occa::memory(&mem);
    }
  }
}
