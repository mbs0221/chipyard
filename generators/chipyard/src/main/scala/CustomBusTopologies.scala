
package chipyard

import freechips.rocketchip.config.{Field, Config, Parameters}
import freechips.rocketchip.diplomacy._
import freechips.rocketchip.tilelink._
import freechips.rocketchip.util.{Location, Symmetric}
import freechips.rocketchip.subsystem._

// I'm putting this code here temporarily as I think it should be a candidate
// for upstreaming based on input from Henry Cook, but don't wnat to deal with
// an RC branch just yet.

// For subsystem/BusTopology.scala

/**
  * Keys that serve as a means to define crossing types from a Parameters instance
  */
case object SubsystemCrossingParamsKey extends Field[SubsystemCrossingParams](SubsystemCrossingParams())
case object MemoryBusCrossingTypeKey extends Field[ClockCrossingType](NoCrossing)

// Biancolin: This, modified from Henry's email
/** Parameterization of a topology containing a banked coherence manager and a bus for attaching memory devices. */
case class CoherentBusTopologyParams(
  sbus: SystemBusParams, // TODO remove this after better width propagation
  mbus: MemoryBusParams,
  l2: BankedL2Params,
  sbusToMbusXType: ClockCrossingType = NoCrossing
) extends TLBusWrapperTopology(
  instantiations = (if (l2.nBanks == 0) Nil else List(
    (MBUS, mbus),
    (L2, CoherenceManagerWrapperParams(mbus.blockBytes, mbus.beatBytes, l2.nBanks, L2.name)(l2.coherenceManager)))),
  connections = if (l2.nBanks == 0) Nil else List(
    (SBUS, L2,   TLBusWrapperConnection(xType = NoCrossing, driveClockFromMaster = Some(true), nodeBinding = BIND_STAR)()),
    (L2,  MBUS,  TLBusWrapperConnection.crossTo(
      xType = sbusToMbusXType,
      driveClockFromMaster = Some(true),
      nodeBinding = BIND_QUERY))
  )
)

// For subsystem/Configs.scala

class WithCoherentBusTopology extends Config((site, here, up) => {
  case TLNetworkTopologyLocated(InSubsystem) => List(
    JustOneBusTopologyParams(sbus = site(SystemBusKey)),
    HierarchicalBusTopologyParams(
      pbus = site(PeripheryBusKey),
      fbus = site(FrontBusKey),
      cbus = site(ControlBusKey),
      xTypes = SubsystemCrossingParams()),
    CoherentBusTopologyParams(
      sbus = site(SystemBusKey),
      mbus = site(MemoryBusKey),
      l2 = site(BankedL2Key),
      sbusToMbusXType = site(MemoryBusCrossingTypeKey)))
})

/**
  * Mixins to specify crossing types between the 5 traditional TL buses
  *
  * Note: these presuppose the legacy connections between buses and set
  * parameters in SubsystemCrossingParams; they may not be resuable in custom
  * topologies (but you can specify the desired crossings in your topology).
  *
  * @param xType The clock crossing type
  *
  */
class WithMemoryBusCrossingType(xType: ClockCrossingType) extends Config((site, here, up) => {
    case MemoryBusCrossingTypeKey => xType
})

class WithFrontBusCrossingType(xType: ClockCrossingType) extends Config((site, here, up) => {
    case SubsystemCrossingParamsKey => up(SubsystemCrossingParamsKey, site)
      .copy(fbusToSbusXType = xType)
})

class WithControlBusCrossingType(xType: ClockCrossingType) extends Config((site, here, up) => {
    case SubsystemCrossingParamsKey => up(SubsystemCrossingParamsKey, site)
      .copy(sbusToCbusXType = xType)
})

class WithPeripheryBusCrossingType(xType: ClockCrossingType) extends Config((site, here, up) => {
    case SubsystemCrossingParamsKey => up(SubsystemCrossingParamsKey, site)
      .copy(cbusToPbusXType = xType)
})

/**
  * Mixins to set the dtsFrequency field of BusParams -- these will percolate it'st way
  * through the diplomatic clock graph to the clock sources.
  */
class WithPeripheryBusFrequency(freq: BigInt) extends Config((site, here, up) => {
  case PeripheryBusKey => up(PeripheryBusKey).copy(dtsFrequency = Some(freq))
})
class WithMemoryBusFrequency(freq: BigInt) extends Config((site, here, up) => {
  case MemoryBusKey => up(MemoryBusKey).copy(dtsFrequency = Some(freq))
})

class WithRationalMemoryBusCrossing extends WithMemoryBusCrossingType(RationalCrossing(Symmetric))
class WithAsynchrousMemoryBusCrossing extends WithMemoryBusCrossingType(AsynchronousCrossing())