/**
 * @file selective_dispatcher_capi.h
 * @brief C Application Binary Interface (ABI) for AXIOM Core
 * * Bu dosya Rust, Python (ctypes/cffi) veya C# tarafından doğrudan okunabilir.
 * İçinde C++'a ait (class, std::vector, Eigen) HİÇBİR ŞEY olamaz!
 */

#pragma once

#ifndef SELECTIVE_DISPATCHER_CAPI_H
#define SELECTIVE_DISPATCHER_CAPI_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" { // Derleyiciye "İsimleri bozma (Name Mangling yapma), saf C gibi derle" emri
#endif

// OPAQUE POINTER (Opak İşaretçi) - Axiom motorunun dış dünyadaki gölgesi
typedef void* AxiomDispatcherHandle;

// --- YAŞAM DÖNGÜSÜ (LIFECYCLE) ---
// Motoru başlatır ve hafıza adresini verir
AxiomDispatcherHandle Axiom_CreateDispatcher();

// Motoru yok eder ve hafızayı işletim sistemine iade eder
void Axiom_DestroyDispatcher(AxiomDispatcherHandle handle);


// --- SIFIR-KOPYALAMA (ZERO-COPY) VERİ KÖPRÜSÜ ---
// Dış dünyadan gelen ham 'double' dizisini motorun içine fırlatır
// Return: 0 (Başarılı), -1 (Geçersiz Pointer), -2 (C++ Çekirdek Çöküşü)
int Axiom_DispatchMatrix(AxiomDispatcherHandle handle, 
                         const char* operation_name, 
                         const double* raw_data, 
                         size_t rows, 
                         size_t cols);

#ifdef __cplusplus
}
#endif

#endif // SELECTIVE_DISPATCHER_CAPI_H