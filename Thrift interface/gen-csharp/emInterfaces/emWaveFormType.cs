/**
 * Autogenerated by Thrift Compiler (0.9.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */

namespace emInterfaces
{
  /// <summary>
  /// Standard types of waveform that the signal generator can produce
  /// </summary>
  public enum emWaveFormType
  {
    /// <summary>
    /// Undefined, here for consistency
    /// </summary>
    emNULL = 0,
    ARBITRARY = 1,
    PWM = 2,
    SAW = 3,
    SINE = 4,
  }
}