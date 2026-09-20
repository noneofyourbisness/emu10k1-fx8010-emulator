#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace fx8010 {

inline constexpr const char *kCoreVersion = "1.27.0";

struct Instruction { std::uint8_t op{}; std::uint16_t a{},x{},y{},r{}; };
struct GprInit { std::uint16_t reg{}; std::int32_t value{}; };
struct TramDecl {
    // First five fields intentionally retain the original shared-engine aggregate
    // order so older factory_programs.h files remain source-compatible.
    std::uint16_t dataReg{};
    bool write{};
    std::uint32_t initialAddress{};
    // Creative FX8010 RIFX delay declarations preserve a relocation-independent
    // delay offset here for reads. Across all 11 TRAM-using July-1999 SBLFX
    // programs, initialAddress - auxiliary equals a declared write-base address
    // in the same tank domain. Writes carry zero.
    std::uint32_t auxiliary{};
    std::uint16_t patchSite{};
    std::uint16_t resource{};      // raw Creative RIFX resource word
    bool external{false};          // false=ITRAM, true=XTRAM/ETRAM
    // Physical tank-access slot allocated within the corresponding ITRAM or
    // XTRAM domain. RIFX declarations are serialized internal-first/external-last,
    // so this can be recovered without assigning host/card register addresses.
    std::uint16_t hardwareSlot{};
    // ALIGN bit produced by the recovered EMUAPS.VXD loader equations for the
    // declaration's patched DSP instruction and physical tank slot.
    bool align{};
};
struct PatchEntry { std::uint16_t virtualReg{}, site{}; };
struct TramHardwareControl {
    std::uint32_t address{}; // physical 20-bit tank address
    bool clear{};
    bool align{};
    bool write{};
    bool read{};
};
struct TramTank20Bucket {
    std::int32_t low{};
    std::int32_t high{};
};

// Creative US 6,032,235 explicitly defines internal-TRAM RSAW as:
//   old = TRAM[address]; sum = old + dataBuffer; TRAM[address] = sum.
// The patent does not state the 20-bit arithmetic overflow law.  Preserve that
// last silicon ambiguity by reporting both natural candidates rather than
// silently choosing wrap or saturation.  When `clearForcedZero` is true the
// patent's RSAW clearing mux writes zero instead of either sum candidate.
struct TramRsaw20Result {
    std::int32_t memoryBefore{};
    std::int32_t dataBuffer{};
    std::int64_t mathematicalSum{};
    std::int32_t wrap20{};
    std::int32_t saturate20{};
    bool overflow{};
    bool clearForcedZero{};
    std::int32_t effectiveWriteIfWrap{};
    std::int32_t effectiveWriteIfSaturate{};
    std::uint16_t rawWordIfWrap{};
    std::uint16_t rawWordIfSaturate{};
};

// Source-backed EMU10K1 internal-TRAM initialization controller from Creative
// US 6,032,235.  The hardware owns one global zeroed-samples counter while
// each clearable tank access carries a programmed countdown length.  READ
// presents zero until equality.  Internal RSAW instead writes zero and blocks
// microcode writes to that access's data buffer while CLR is active.
//
// This model is deliberately separate from RIFX TramDecl: the public RIFX
// effects recovered so far do not serialize the CLR/countdown state needed to
// drive it.  It is a controller/reference path for host-driver reconstruction.
enum class TramClearKind : std::uint8_t {
    Off,
    Read,
    InternalRsaw,
};

struct TramClearSlotState {
    TramClearKind kind{TramClearKind::Off};
    std::uint32_t countdownLength{};
    bool enabled{};
    bool clearActive{};
    bool readReturnsZero{};
    bool rsawForcesZeroWrite{};
    bool microcodeDataBufferWriteAllowed{true};
};

class TramClearController {
public:
    static constexpr unsigned kSlotCount = 160u;
    static constexpr unsigned slotCount() noexcept { return kSlotCount; }
    void reset(std::uint32_t zeroedSamples=0u) noexcept;
    void configure(unsigned slot, TramClearKind kind, std::uint32_t countdownLength, bool enabled=true) noexcept;
    void disable(unsigned slot) noexcept;
    void setZeroedSamples(std::uint32_t samples) noexcept { zeroedSamples_=samples; }
    std::uint32_t zeroedSamples() const noexcept { return zeroedSamples_; }
    void advanceSample() noexcept;
    TramClearSlotState state(unsigned slot) const noexcept;
private:
    struct Slot {
        TramClearKind kind{TramClearKind::Off};
        std::uint32_t countdownLength{};
        bool enabled{};
    };
    std::array<Slot,kSlotCount> slots_{};
    std::uint32_t zeroedSamples_{};
};

struct XtramServiceRequest {
    std::uint8_t cache{};
    bool write{};
    std::uint32_t currentAddress{};
    std::uint32_t burstStart{};
    std::uint64_t selectedSample{};
    std::uint64_t headroomDeadlineSample{}; // selected + (M-B) = +2
    std::uint64_t retrySelectionSample{};   // strictly later round-robin visit = +16
    std::uint64_t mappingGeneration{};      // external-tank TCB/TCBS mapping epoch
};

struct XtramServiceDecision {
    unsigned cache{};
    bool active{};
    bool write{};
    bool admitted{};
    bool dropped{};
    bool blockedByTankLock{};
    std::uint32_t currentAddress{};
    std::uint32_t burstStart{};
    std::uint64_t selectedSample{};
    std::uint64_t headroomDeadlineSample{};
    std::uint64_t retrySelectionSample{};
};

struct XtramServiceSelection {
    std::array<unsigned,2> selected{};
    std::array<XtramServiceDecision,2> decisions{};
    unsigned enqueued{};
    unsigned dropped{};
    unsigned blockedByTankLock{};
};

struct XtramServiceCompletion {
    XtramServiceRequest request{};
    std::uint64_t completedSample{};
    unsigned ageSamples{};
    unsigned beyondHeadroomSamples{};
    bool beyondHeadroom{};
};

// US 6,275,899 names the two caches selected in a sample but does not specify
// an intra-pair winner when exactly one FIFO entry is free and both caches are
// active.  Keep that one silicon detail explicit instead of baking an arbitrary
// lower-index priority into the controller.
enum class XtramPairOrder : std::uint8_t {
    LowerThenUpper,
    UpperThenLower,
};

struct XtramAdmissionEnvelope {
    std::array<unsigned,2> selected{};
    unsigned active{};
    unsigned freeEntries{};
    unsigned enqueued{};       // count is source-determined even when identity is not
    unsigned dropped{};
    bool identityAmbiguous{};  // true only for 2 active requests / exactly 1 free entry
};

// Timing view of the source-traced external-delay service queue.  US 6,275,899
// gives an 18-sample cache with 16-sample transfers, so requests are made two
// sample periods before a read cache would empty / write cache would fill.
// `beyondLatencyBudget` therefore means the patented two-sample tolerance has
// been exceeded.  It is a controller-risk diagnostic, not an assertion of the
// exact audible sample produced after a late PCI completion.
struct XtramServiceTimingState {
    unsigned depth{};
    unsigned oldestAge{};
    unsigned withinLatencyBudget{}; // age 0..2 inclusive
    unsigned atLatencyLimit{};      // age == 2
    unsigned beyondLatencyBudget{}; // age > 2
};

// Source-traced EMU10K1-era external-delay cache request FIFO model from
// Creative US 6,275,899.  It models only the patented cache-selection and
// request-queue controller: 32 caches, pairs 0/1..30/31 selected by a 4-bit
// sample counter, four FIFO entries, FIFO-order completion, and drop-on-full.
// PCI completion latency is deliberately supplied by the caller instead of
// guessed, so this model is suitable for conformance/fault-injection research
// without injecting speculative timing into the ordinary sample-level engine.
class XtramServiceFifo {
public:
    void reset() noexcept;
    void configure(unsigned cache, bool active, bool write, std::uint32_t currentAddress) noexcept;
    // Creative-derived HCFG documentation names LOCKTANKCACHE as cancelling
    // tank-cache bus-master accesses.  The old Creative OSS path and ALSA both
    // assert it before invalidating/freeing TCB/TCBS, then unlock after the new
    // coherent DMA mapping is installed.  Model this as a controller request
    // barrier; it deliberately does NOT claim that the on-chip 18-sample cache
    // payload is itself cleared.
    void setTankCacheLocked(bool locked) noexcept;
    bool tankCacheLocked() const noexcept { return tankCacheLocked_; }
    void programTankMapping(std::uint32_t tcbBase, unsigned tcbsCode, bool valid=true) noexcept;
    std::uint32_t tankBase() const noexcept { return tankBase_; }
    unsigned tankSizeCode() const noexcept { return tankSizeCode_; }
    bool tankMappingValid() const noexcept { return tankMappingValid_; }
    std::uint64_t mappingGeneration() const noexcept { return mappingGeneration_; }
    std::uint64_t cancelledCount() const noexcept { return cancelled_; }
    std::uint64_t lockSuppressedCount() const noexcept { return lockSuppressed_; }
    static constexpr std::uint32_t samplesForTcbsCode(unsigned code) noexcept {
        return 0x2000u << (code & 7u);
    }
    XtramAdmissionEnvelope admissionEnvelope(std::uint64_t sampleCounter) const noexcept;
    XtramServiceSelection select(std::uint64_t sampleCounter, XtramPairOrder order=XtramPairOrder::LowerThenUpper) noexcept;
    bool completeOne(XtramServiceRequest* completed=nullptr) noexcept;
    // Completion-timestamp variant.  This does not guess PCI latency; the caller
    // supplies the completion sample and the model reports whether the patented
    // M-B=2 cache headroom has been exceeded.
    bool completeOneAt(std::uint64_t completedSample, XtramServiceCompletion* completed=nullptr) noexcept;
    unsigned complete(unsigned count) noexcept;
    unsigned completeAt(std::uint64_t completedSample, unsigned count, unsigned* late=nullptr) noexcept;
    unsigned depth() const noexcept { return size_; }
    bool empty() const noexcept { return size_==0; }
    bool full() const noexcept { return size_==fifo_.size(); }
    std::uint64_t acceptedCount() const noexcept { return accepted_; }
    std::uint64_t droppedCount() const noexcept { return dropped_; }
    std::uint64_t completedCount() const noexcept { return completed_; }
    const XtramServiceRequest* front() const noexcept;
    unsigned oldestAgeSamples(std::uint64_t now) const noexcept;
    XtramServiceTimingState timingState(std::uint64_t now) const noexcept;
    static std::uint64_t nextSelectionAfter(unsigned cache, std::uint64_t selectedSample) noexcept;
    static constexpr unsigned transferSamples() noexcept { return 16u; }
    static constexpr unsigned cacheSamples() noexcept { return 18u; }
    static constexpr unsigned servicePeriodSamples() noexcept { return 16u; }
    static constexpr unsigned patentLatencySlackSamples() noexcept { return cacheSamples()-transferSamples(); }
    static constexpr unsigned fifoCapacity() noexcept { return 4u; }
    // If a request is dropped at its selection point, its next possible request
    // is +16 samples while the cache had only two samples of headroom. Even with
    // an instantaneous retry completion, at least 14 sample periods lie beyond
    // the source-backed latency budget. This is a service-risk bound, not an
    // assertion of what stale/corrupt sample the silicon outputs.
    static constexpr unsigned minimumDroppedHeadroomGapSamples() noexcept {
        return servicePeriodSamples()-patentLatencySlackSamples();
    }
private:
    std::array<bool,32> active_{};
    std::array<bool,32> write_{};
    std::array<std::uint32_t,32> currentAddress_{};
    std::array<XtramServiceRequest,4> fifo_{};
    unsigned head_{};
    unsigned size_{};
    std::uint64_t accepted_{};
    std::uint64_t dropped_{};
    std::uint64_t completed_{};
    std::uint64_t cancelled_{};
    std::uint64_t lockSuppressed_{};
    bool tankCacheLocked_{};
    bool tankMappingValid_{};
    std::uint32_t tankBase_{};
    unsigned tankSizeCode_{};
    std::uint64_t mappingGeneration_{};
};

enum class TramAccessMode : std::uint8_t {
    Off,
    Read,
    Write,
    ReadSumWrite,
};
struct Program {
    bool bigEndian{true};
    std::string target, version, name;
    std::array<std::uint16_t,13> rsrc{};
    std::uint32_t itramSize{};     // RIFX-declared samples
    std::uint32_t xtramSize{};     // RIFX-declared samples
    std::vector<GprInit> gprInit;
    std::vector<TramDecl> tram;
    std::vector<std::uint16_t> inputPatchSites;
    std::vector<PatchEntry> outputs;
    std::vector<Instruction> code;
    // Compatibility escape hatch for legacy effect reconstructions which were
    // tuned before the physical 16-bit logarithmic TRAM formatter was modeled.
    // False is the hardware-oriented default; do not use for new programs.
    bool linearTramCompatibility{false};
    // Native RIFX programs opt into the independently-clocked DSP/TRAM schedule
    // recovered from the EMUAPS loader equations. Hand-authored compatibility
    // programs retain the older begin/read -> DSP -> end/write abstraction.
    bool scheduledTramSequencer{false};
};

class RifxLoader {
public:
    static bool loadFile(const std::string& path, Program& out, std::string* error=nullptr);
    static bool loadBytes(const std::uint8_t* data, std::size_t size, Program& out, std::string* error=nullptr);
};

class Engine {
public:
    Engine();
    void load(const Program& p, std::size_t minimumTramSamples=0);
    void reset(bool keepProgramDefaults=true);
    void runSample();

    // Optional full-card I/O view for EMU10K1.  Real-card experiments show
    // automatic FXBUS->EXTOUT and EXTIN->FXBUS2 routing when microcode does
    // not write an output; any microcode write to that output overrides it.
    void runCardSample(const std::array<std::int32_t,16>& fxbus,
                       const std::array<std::int32_t,16>& extin,
                       std::array<std::int32_t,32>& outputs);

    void set(std::uint16_t reg, std::int32_t value) noexcept;
    std::int32_t get(std::uint16_t reg) const noexcept;
    void setTramAddressRaw(std::uint16_t dataReg, std::int32_t address21_11) noexcept;
    void setTramAddress(std::uint16_t dataReg, double delaySamples) noexcept;
    std::size_t tramSize() const noexcept { return itramMem_.size()+xtramMem_.size(); }
    std::size_t itramSize() const noexcept { return itramMem_.size(); }
    std::size_t xtramSize() const noexcept { return xtramMem_.size(); }
    std::uint32_t debugDbac() const noexcept { return dbac_ & 0x000fffffu; }
#ifdef SBLIVE_TESTING
    std::uint32_t debugLastTramRelative(std::uint16_t dataReg) const noexcept { return debugLastTramRelative_[dataReg]; }
#endif
    // EMU10K1 debug register exposes whether arithmetic saturation occurred
    // and the microinstruction address which produced it. Wrap-only S events
    // are intentionally not reported through this diagnostic latch.
    bool saturationOccurred() const noexcept { return saturationOccurred_; }
    std::uint16_t saturationAddress() const noexcept { return saturationAddress_; }
    void clearSaturation() noexcept { saturationOccurred_=false; saturationAddress_=0; }
    // EMU10K1 DBG register subset documented by ALSA.  This exposes the
    // hardware-visible saturation latch/address and current five-bit CCR.
    // Single-step controls remain unimplemented rather than guessed.
    std::uint32_t debugRegisterWord() const noexcept;
    void writeDebugRegister(std::uint32_t word) noexcept;
    // FX8010 GPR_IRQ is a write-side-effect register.  The Linux stock FX
    // microcode raises the DSP IRQ by writing 0x80000000 to it.
    bool irqPending() const noexcept { return irqPending_; }
    bool consumeIrq() noexcept { const bool p=irqPending_; irqPending_=false; return p; }
    std::uint64_t irqCount() const noexcept { return irqCount_; }

    // E-mu APS 1.5 EMUAPS.VXD EMU10K1 loader ALIGN calculation, exposed
    // only as a diagnostic.  The loader computes this after both code and
    // tank taps have been relocated to their final physical positions.
    // The normal interpreter remains at the programmer-visible sample-level
    // abstraction and therefore does not insert an extra delay for ALIGN.
    static bool tramAlignFlag(bool external, bool write, unsigned instructionIndex, unsigned slot) noexcept;

    // Hardware-tested FX8010 SKIP condition-word decoder.  The 30 low bits
    // encode three 10-bit boolean brackets over the five CCR bits; the top two
    // bits choose how those brackets are combined.
    static bool skipConditionWord(std::uint32_t ccr, std::uint32_t conditionWord) noexcept;

    // Raw EMU10K1 microinstruction packing used by the physical 512-slot DSP.
    // Word 0 carries X/Y; word 1 carries opcode/result/A.
    static std::array<std::uint32_t,2> encodeInstructionWords(const Instruction&) noexcept;
    static Instruction decodeInstructionWords(std::uint32_t lowWord, std::uint32_t highWord) noexcept;

    // Physical EMU10K1 tank-address register view (128 ITRAM + 32 ETRAM slots).
    // This is card/controller metadata and is deliberately separate from the
    // sample-level TRAM abstraction used by the effect interpreter.
    static constexpr unsigned internalTramAccessSlots() noexcept { return 128u; }
    static constexpr unsigned externalTramAccessSlots() noexcept { return 32u; }
    static constexpr unsigned totalTramAccessSlots() noexcept { return 160u; }
    static std::uint32_t encodeTramHardwareControl(const TramHardwareControl&) noexcept;
    static TramHardwareControl decodeTramHardwareControl(std::uint32_t word) noexcept;
    // Creative's EMU10K1 patents name all four two-bit tank modes explicitly:
    // OFF, READ, WRITE and read-sum-and-write (RSAW).  The public register map
    // exposes independent READ/WRITE bits, making READ|WRITE the RSAW state.
    static TramAccessMode tramAccessMode(const TramHardwareControl&) noexcept;
    // Source-exact internal-TRAM RSAW topology with explicitly unresolved 20-bit
    // overflow law. Inputs are signed 20-bit tank-domain values; out-of-range
    // inputs are clamped to the representable tank domain before the sum.
    static TramRsaw20Result tramInternalRsaw20(std::int32_t memoryTank20,
                                                std::int32_t dataBufferTank20,
                                                bool clearForcedZero=false) noexcept;
    // ALIGN is an address correction for the relative phase of the independent
    // DSP and TRAM sequencers: -1 sample for an aligned read, +1 for a write.
    // These helpers expose the documented physical-address law without forcing
    // cycle scheduling into the sample-level effect interpreter.
    static std::uint32_t tramPhysicalReadAddress(std::uint32_t relative,
                                                  std::uint32_t dbac,
                                                  bool align) noexcept;
    static std::uint32_t tramPhysicalWriteAddress(std::uint32_t relative,
                                                   std::uint32_t dbac,
                                                   bool align) noexcept;
    // CLEAR uses a global zeroed-samples counter. Until the counter reaches the
    // declared delay length, reads are forced to zero. RSAW initialization also
    // forces zero writes while the corresponding countdown is active.
    static bool tramClearReadReturnsZero(std::uint32_t zeroedSamples,
                                         std::uint32_t delayLength) noexcept;
    static bool tramClearRsawForcesZero(std::uint32_t zeroedSamples,
                                        std::uint32_t countdownLength) noexcept;
    // OFF-mode ITRAM address/data buffers are persistent 20-bit storage on the
    // EMU10K1 and may be allocated by the loader as pseudo-GPRs.
    static std::uint32_t tramBuffer20(std::uint32_t value) noexcept;

    // Real-card measurements by michgz show the host PCM path is not unity-scaled:
    // default 16-bit playback lands at roughly -12 dB internally while capture
    // simply takes the top 16 bits. These helpers model the documented/observed
    // integer law only; unexplained low-bit playback variation is not invented.
    static std::int32_t approximatePlayback16ToBus(std::int16_t sample,
                                                    std::uint16_t volume=65535u,
                                                    std::uint8_t sendVolume=255u) noexcept;
    static std::int16_t capture16FromBus(std::int32_t sample) noexcept;

    static std::int32_t q31(double v) noexcept;
    static double fromQ31(std::int32_t v) noexcept;
    static std::int32_t sat32(std::int64_t v) noexcept;
    static std::int32_t logEncode(std::int32_t linear, std::uint32_t maxExponent, std::uint32_t signMode) noexcept;
    static std::int32_t expDecode(std::int32_t encoded, std::uint32_t maxExponent, std::uint32_t signMode) noexcept;
    // EMU10K1 physical TRAM codeword law recovered from the stock Linux raw
    // IEC958 FX8010 program.  The raw path must undo hardware XTRAM expansion
    // without modifying arbitrary 16-bit PCM words; its exact sequence is
    // LOG(7,0), keep the upper 16 bits, then XOR 0x7000 for negative words.
    // This establishes the raw 16-bit XTRAM codeword/recovery mapping.
    static std::uint16_t tramRawWordFromTank20(std::int32_t tank20) noexcept;

    // DSP-facing representative for a physical raw word.  After undoing the
    // physical negative-word 0x7000 transform, discarded positive LOG bits are
    // restored as zero while discarded negative LOG bits are restored as one.
    // That follows Creative US 5,930,158 EXP semantics: a negative LOG operand
    // is one's-complemented first, the positive mantissa is zero-filled, and
    // the final linear result is complemented back.  The result is the low edge
    // of positive buckets and the high edge of negative buckets (sign-symmetric
    // truncation toward zero in magnitude).
    static std::int32_t tramTank20RepresentativeFromRawWord(std::uint16_t code) noexcept;
    // Exact signed-20 preimage interval of a raw physical word under the proven
    // LOG7/raw-IEC958 codeword law.  This interval is independent of which
    // member the silicon decompressor presents and makes the remaining uncertainty explicit.
    static TramTank20Bucket tramTank20BucketFromRawWord(std::uint16_t code) noexcept;

    // EMU10K1 external-delay cache geometry traced to Creative US 6,275,899 and
    // the early-revision EMUAPS.VXD correction. These describe the PCI/cache
    // layer beneath the per-sample TRAM sequencer; they are diagnostics and do
    // not add an artificial delay to the effect-level interpreter.
    static constexpr unsigned xtramPciBurstSamples() noexcept { return 16u; }
    static constexpr unsigned xtramDelayCacheSamples() noexcept { return 18u; }
    static constexpr unsigned xtramDelayCacheCount() noexcept { return 32u; }
    static constexpr unsigned xtramCacheServicePeriodSamples() noexcept { return 16u; }
    static constexpr unsigned xtramCachesSelectedPerSample() noexcept { return 2u; }
    static constexpr unsigned xtramServiceFifoDepth() noexcept { return 4u; }
    static std::array<unsigned,2> xtramCachesSelected(std::uint32_t sampleCounter) noexcept;
    static int xtramReadBurstStartAdjustment(std::uint32_t currentAddress) noexcept;
    static int xtramWriteBurstStartAdjustment(std::uint32_t currentAddress) noexcept;

    // Source-compatibility aliases retained for older clients.
    static std::uint16_t tramEncode20Candidate(std::int32_t tank20) noexcept;
    static std::int32_t tramDecode20Candidate(std::uint16_t code) noexcept;

    const Program& program() const noexcept { return program_; }
    std::vector<Instruction>& code() noexcept { return code_; }
    const std::vector<Instruction>& code() const noexcept { return code_; }

private:
    struct Acc { std::uint64_t lo{}; std::uint8_t hi{}; }; // signed 67-bit two's-complement modulo 2^67
    void initConstants() noexcept;
    void execute(const Instruction&) noexcept;
    bool isTramAddressReg(std::uint16_t reg) const noexcept { return tramAddress_[reg]; }
    std::int32_t readOperand(std::uint16_t reg) const noexcept;
    std::int32_t readA(std::uint16_t reg, std::uint8_t op) const noexcept;
    void updateCCR(std::int32_t result, bool saturated, bool borrow=false) noexcept;
    void updateCCRMasked(std::int32_t result, bool saturated, bool borrow, std::uint32_t mask) noexcept;
    void tramBegin() noexcept;
    void tramEnd() noexcept;
    void tramScheduledBegin() noexcept;
    void tramScheduledServiceBefore(unsigned pc) noexcept;
    void tramScheduledFinish() noexcept;
    void tramReadService(TramDecl& t, bool physicalAlign) noexcept;
    void tramWriteService(TramDecl& t, bool physicalAlign) noexcept;
    static unsigned tramReadServicePc(bool external, unsigned slot) noexcept;
    static unsigned tramWriteServicePc(bool external, unsigned slot) noexcept;
    static std::int64_t highAccRaw(Acc a) noexcept;
    static std::int32_t highAcc(Acc a) noexcept;
    static std::int32_t highAccWrap(Acc a) noexcept;
    static std::int32_t lowAcc(Acc a) noexcept;
    static Acc wrap67(Acc a) noexcept;
    static Acc accFromI64(std::int64_t v) noexcept;
    static Acc accFromShift31(std::int64_t v) noexcept;
    static Acc accAdd(Acc a, Acc b) noexcept;
    static std::int32_t mulHigh(std::int32_t x,std::int32_t y) noexcept;
    static std::int32_t q31ToTank20(std::int32_t q31) noexcept;
    static std::int32_t tank20ToQ31(std::int32_t tank20) noexcept;
    static std::uint32_t nextNoiseWord(std::uint32_t& state) noexcept;
    static std::int32_t noiseQ31(std::uint32_t& state) noexcept;

    Program program_{};
    std::array<std::int32_t,65536> reg_{};
    std::array<bool,65536> constant_{};
    std::array<bool,65536> tramAddress_{};
    std::vector<Instruction> code_;
    std::vector<TramDecl> tram_;
    std::vector<std::uint16_t> itramMem_;
    std::vector<std::uint16_t> xtramMem_;
    std::vector<std::int32_t> itramLinearCompat_;
    std::vector<std::int32_t> xtramLinearCompat_;
    std::size_t minimumTramSamples_{};
    std::uint32_t dbac_{};
    // Public sources agree that NOISE0/NOISE1 are FX8010 noise sources, but the
    // exact silicon generator, seed, and relationship between them remain
    // unresolved.  The existing 4097-sample NOISE0->NOISE1 delay is retained
    // only as a compatibility hypothesis for previously tuned reconstructions;
    // it must not be treated as independently proven bit-exact behavior.
    static constexpr std::size_t kNoiseDelaySamples = 4097u;
    std::uint32_t noiseState0_{0x13579bdfu};
    std::array<std::int32_t,kNoiseDelaySamples> noiseDelay_{};
    std::size_t noiseDelayPos_{};
    Acc acc_{};
    unsigned skip_{};
    bool irqPending_{};
    std::uint64_t irqCount_{};
    bool saturationOccurred_{};
    std::uint16_t saturationAddress_{};
    std::uint16_t currentPc_{};
    bool cardMode_{};
    std::array<bool,32> cardOutputWritten_{};
#ifdef SBLIVE_TESTING
    std::array<std::uint32_t,65536> debugLastTramRelative_{};
#endif
};

bool patchSite(std::vector<Instruction>& code,std::uint16_t site,std::uint16_t replacement) noexcept;

} // namespace fx8010
