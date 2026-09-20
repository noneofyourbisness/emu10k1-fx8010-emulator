cmake_minimum_required(VERSION 3.20)
project(FX8010AccuracyCore1270 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_library(fx8010_engine STATIC src/fx8010_engine.cpp)
target_include_directories(fx8010_engine PUBLIC src)
if(MSVC)
  target_compile_options(fx8010_engine PRIVATE /W4 /permissive- /EHsc)
else()
  target_compile_options(fx8010_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()
enable_testing()
foreach(t ccr_layout accumulator_edges macintw_wrap macint_accu_lowword accum_guard_reuse acc3_accum accum_result_pipeline readonly_specials borrow_independent skip_predicates skip_hardware_semantics constant_map tram_domains tram_fractional tram_read_before_write tram_scheduled_sequencer tram_align_formula tram_log_relation tram_raw_spdif_codebook tram_decompressor_patent xtram_cache_geometry xtram_fifo_controller xtram_fifo_ambiguity xtram_pci_state xtram_service_deadlines xtram_tankcache_lock tram_rsaw_exact tram_address_view irq_register saturation_debug rifx_resources rifx_tram_phase_metadata logexp_roundtrip log_reference_vectors noise_sources dbac_cadence ccr_result_matrix macmv_accumulator accu_xy_zero logexp_michgz_vectors skip_michgz_vectors tstneg_michgz_ccr instruction_words card_default_routing mac23_michgz_vectors michgz_remaining_opcode_sweep tram_hardware_control card_pcm_scaling pt_playback_controlflow debug_register_card recovered_suite_audit tram_log7_vectors tram_linear_compat native_record_dither native_peak_meter native_ac3pass native_sblfx_suite_audit native_sblfx_smoke native_tram_delay_geometry native_spatializer_tram_schedule native_spatializer_signal_response native_spatializer_host_feeds noise_cadence_evidence native_tram_address_phase native_dynamic_tram_pipeline tram_patent_modes tram_patent_align tram_clear_patent tram_clear_controller tram_off_buffer_gpr skip_patent_conflict logexp_patent_edges full_factory_corpus_audit full_factory_corpus_smoke noise_delay_4097 full_factory_corpus_dynamic pci1024_liveware_revision liveware_balance_law native_surround_feed_taxonomy)
  add_executable(${t} tests/${t}.cpp)
  target_link_libraries(${t} PRIVATE fx8010_engine)
  if(MSVC)
    target_compile_options(${t} PRIVATE /W4 /permissive- /EHsc)
  else()
    target_compile_options(${t} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endforeach()
add_test(NAME ccr_layout COMMAND ccr_layout)
add_test(NAME accumulator_edges COMMAND accumulator_edges)
add_test(NAME macintw_wrap COMMAND macintw_wrap)
add_test(NAME macint_accu_lowword COMMAND macint_accu_lowword)
add_test(NAME accum_guard_reuse COMMAND accum_guard_reuse)
add_test(NAME acc3_accum COMMAND acc3_accum)
add_test(NAME accum_result_pipeline COMMAND accum_result_pipeline)
add_test(NAME readonly_specials COMMAND readonly_specials)
add_test(NAME borrow_independent COMMAND borrow_independent)
add_test(NAME skip_predicates COMMAND skip_predicates)
add_test(NAME skip_hardware_semantics COMMAND skip_hardware_semantics)
add_test(NAME constant_map COMMAND constant_map)
add_test(NAME tram_domains COMMAND tram_domains)
add_test(NAME tram_fractional COMMAND tram_fractional)
add_test(NAME tram_read_before_write COMMAND tram_read_before_write)
add_test(NAME tram_scheduled_sequencer COMMAND tram_scheduled_sequencer)
add_test(NAME tram_align_formula COMMAND tram_align_formula)
add_test(NAME tram_log_relation COMMAND tram_log_relation)
add_test(NAME tram_raw_spdif_codebook COMMAND tram_raw_spdif_codebook)
add_test(NAME tram_decompressor_patent COMMAND tram_decompressor_patent)
add_test(NAME xtram_cache_geometry COMMAND xtram_cache_geometry)
add_test(NAME xtram_fifo_controller COMMAND xtram_fifo_controller)
add_test(NAME xtram_fifo_ambiguity COMMAND xtram_fifo_ambiguity)
add_test(NAME xtram_pci_state COMMAND xtram_pci_state)
add_test(NAME xtram_service_deadlines COMMAND xtram_service_deadlines)
add_test(NAME xtram_tankcache_lock COMMAND xtram_tankcache_lock)
add_test(NAME tram_rsaw_exact COMMAND tram_rsaw_exact)
add_test(NAME tram_address_view COMMAND tram_address_view)
add_test(NAME irq_register COMMAND irq_register)
add_test(NAME saturation_debug COMMAND saturation_debug)
add_test(NAME rifx_resources COMMAND rifx_resources
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/Chorus.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/Panning_Echo.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/FDNReverb_EAX2.rifx")
add_test(NAME rifx_tram_phase_metadata COMMAND rifx_tram_phase_metadata
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/factory_aps15"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/factory_liveware25")
add_test(NAME logexp_roundtrip COMMAND logexp_roundtrip)
add_test(NAME log_reference_vectors COMMAND log_reference_vectors)

add_test(NAME noise_sources COMMAND noise_sources)
add_test(NAME dbac_cadence COMMAND dbac_cadence)
add_test(NAME ccr_result_matrix COMMAND ccr_result_matrix)
add_test(NAME macmv_accumulator COMMAND macmv_accumulator)
add_test(NAME accu_xy_zero COMMAND accu_xy_zero)

add_test(NAME logexp_michgz_vectors COMMAND logexp_michgz_vectors)
add_test(NAME skip_michgz_vectors COMMAND skip_michgz_vectors)
add_test(NAME tstneg_michgz_ccr COMMAND tstneg_michgz_ccr)
add_test(NAME instruction_words COMMAND instruction_words)
add_test(NAME card_default_routing COMMAND card_default_routing)
add_test(NAME mac23_michgz_vectors COMMAND mac23_michgz_vectors)

add_test(NAME michgz_remaining_opcode_sweep COMMAND michgz_remaining_opcode_sweep)
add_test(NAME tram_hardware_control COMMAND tram_hardware_control)
add_test(NAME card_pcm_scaling COMMAND card_pcm_scaling)
add_test(NAME pt_playback_controlflow COMMAND pt_playback_controlflow)

add_test(NAME debug_register_card COMMAND debug_register_card)
add_test(NAME recovered_suite_audit COMMAND recovered_suite_audit
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/recovered/Auto_Wah.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/recovered/Chorus.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/recovered/Distortion.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/recovered/Flanger.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/recovered/Pitch_Shifter.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/recovered/Ring_Modulator.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/recovered/Panning_Echo.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/recovered/FDNReverb_EAX2.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/recovered/Vocal_Morpher.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/recovered/Frequency_Shifter.rifx")

add_test(NAME tram_log7_vectors COMMAND tram_log7_vectors)
add_test(NAME tram_linear_compat COMMAND tram_linear_compat)

add_test(NAME native_record_dither COMMAND native_record_dither
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/recovered/Record_Dither.rifx")
add_test(NAME native_peak_meter COMMAND native_peak_meter
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/recovered/Peak_Meter.rifx")
add_test(NAME native_ac3pass COMMAND native_ac3pass
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/recovered/ac3pass.rifx")

add_test(NAME native_sblfx_suite_audit COMMAND native_sblfx_suite_audit
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/2HeadphoneSpatializer.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Auto_Wah.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Chorus.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/FDNReverb_EAX2_0_version.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Formant_Filter.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Freak_Shifter.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Fuzz.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Panning_Echo.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Parametric_EQ.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Peak_Meter.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/PitchShift.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Record_Dither.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/RingMod.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/SB2Mixer.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/SB2SpeakerSpatializer.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/SB4Mixer.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/SB4SpeakerSpatializer.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/SBLTone4.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/SBLTone4_2.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Stereo_Flanger.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/SurroundSpatializer.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Surround_Encoder.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/ac3pass.rifx")
add_test(NAME native_sblfx_smoke COMMAND native_sblfx_smoke
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/2HeadphoneSpatializer.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Auto_Wah.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Chorus.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/FDNReverb_EAX2_0_version.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Formant_Filter.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Freak_Shifter.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Fuzz.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Panning_Echo.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Parametric_EQ.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Peak_Meter.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/PitchShift.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Record_Dither.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/RingMod.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/SB2Mixer.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/SB2SpeakerSpatializer.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/SB4Mixer.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/SB4SpeakerSpatializer.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/SBLTone4.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/SBLTone4_2.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Stereo_Flanger.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/SurroundSpatializer.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Surround_Encoder.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/ac3pass.rifx")


# 1.13 native Creative TRAM/noise evidence tests.
add_test(NAME native_tram_delay_geometry COMMAND native_tram_delay_geometry
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/ac3pass.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Chorus.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Panning_Echo.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/FDNReverb_EAX2_0_version.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Stereo_Flanger.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Freak_Shifter.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/PitchShift.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/SB2SpeakerSpatializer.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/SB4SpeakerSpatializer.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/2HeadphoneSpatializer.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Surround_Encoder.rifx")

add_test(NAME native_spatializer_tram_schedule COMMAND native_spatializer_tram_schedule
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/2HeadphoneSpatializer.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/SB2SpeakerSpatializer.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/SB4SpeakerSpatializer.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Surround_Encoder.rifx")

add_test(NAME native_spatializer_signal_response COMMAND native_spatializer_signal_response
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/2HeadphoneSpatializer.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/SB2SpeakerSpatializer.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/SB4SpeakerSpatializer.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/SurroundSpatializer.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Surround_Encoder.rifx")

add_test(NAME native_spatializer_host_feeds COMMAND native_spatializer_host_feeds
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/2HeadphoneSpatializer.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/SB2SpeakerSpatializer.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/SB4SpeakerSpatializer.rifx")

add_test(NAME noise_cadence_evidence COMMAND noise_cadence_evidence
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Record_Dither.rifx")


add_test(NAME native_tram_address_phase COMMAND native_tram_address_phase
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/2HeadphoneSpatializer.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/SB2SpeakerSpatializer.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/SB4SpeakerSpatializer.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Surround_Encoder.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Chorus.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Stereo_Flanger.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/PitchShift.rifx")


add_test(NAME native_dynamic_tram_pipeline COMMAND native_dynamic_tram_pipeline
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Chorus.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Stereo_Flanger.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/PitchShift.rifx")

# 1.14 patent-derived architectural conformance.
add_test(NAME tram_patent_modes COMMAND tram_patent_modes)
add_test(NAME tram_patent_align COMMAND tram_patent_align)
add_test(NAME tram_clear_patent COMMAND tram_clear_patent)
add_test(NAME tram_clear_controller COMMAND tram_clear_controller)
add_test(NAME tram_off_buffer_gpr COMMAND tram_off_buffer_gpr)
add_test(NAME skip_patent_conflict COMMAND skip_patent_conflict
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Freak_Shifter.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/PitchShift.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/ac3pass.rifx")
add_test(NAME logexp_patent_edges COMMAND logexp_patent_edges)

# 1.15 E-mu APS 1.5 / Creative LiveWare complete factory corpus.
add_test(NAME full_factory_corpus_audit COMMAND full_factory_corpus_audit
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/factory_aps15"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/factory_liveware25")
add_test(NAME full_factory_corpus_smoke COMMAND full_factory_corpus_smoke
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/factory_aps15"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/factory_liveware25")
add_test(NAME noise_delay_4097 COMMAND noise_delay_4097)

add_test(NAME full_factory_corpus_dynamic COMMAND full_factory_corpus_dynamic
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/factory_aps15"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/factory_liveware25")

add_test(NAME pci1024_liveware_revision COMMAND pci1024_liveware_revision "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999" "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/factory_liveware25")

# 1.23 PCI1024 Surround Mixer / native surround feed trace.
add_test(NAME liveware_balance_law COMMAND liveware_balance_law)
add_test(NAME native_surround_feed_taxonomy COMMAND native_surround_feed_taxonomy
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/SurroundSpatializer.rifx"
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native_sblfx_1999/Surround_Encoder.rifx")
