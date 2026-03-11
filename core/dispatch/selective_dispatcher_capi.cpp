/**
 * @file selective_dispatcher_capi.cpp
 * @brief AXIOM FFI Boundary Implementation
 */

#include "selective_dispatcher_capi.h"
#include "selective_dispatcher.h" // Asıl zekamız (Pimpl zırhlı)
#include <Eigen/Core>
#include <iostream>

using namespace AXIOM;

AxiomDispatcherHandle Axiom_CreateDispatcher() {
    try {
        // C++ objesini yaratıp void* olarak dış dünyaya fırlatıyoruz
        return static_cast<void*>(new SelectiveDispatcher());
    } catch (...) {
        return nullptr; // Kritik başlatma hatası
    }
}

void Axiom_DestroyDispatcher(AxiomDispatcherHandle handle) {
    if (handle) {
        // void* olarak gelen gölgeyi tekrar gerçeğe çevirip siliyoruz
        delete static_cast<SelectiveDispatcher*>(handle);
    }
}

int Axiom_DispatchMatrix(AxiomDispatcherHandle handle, 
                         const char* operation_name, 
                         const double* raw_data, 
                         size_t rows, 
                         size_t cols) {
                             
    if (!handle || !operation_name || !raw_data) return -1; // null pointer

    // Validate dimensions before unsafe pointer arithmetic
    if (rows == 0 || cols == 0 || rows > 100000 || cols > 100000) return -1;

    // MİMARIN KURALI: C++ Exception'ları (Hataları) C sınırından DIŞARI SIZAMAZ!
    try {
        auto* dispatcher = static_cast<SelectiveDispatcher*>(handle);
        std::string op_str(operation_name);

        // MİMARIN SİHRİ: EIGEN::MAP (ZERO-COPY)
        // Dışarıdan (örn: Python NumPy'den) gelen raw_data'yı ASLA kopyalamıyoruz.
        // Eigen::Map, o ham RAM bloğunun üzerine sanal bir matris şablonu oturtur.
        Eigen::Map<const Eigen::MatrixXd> mapped_matrix(raw_data, rows, cols);

        // Doğrudan sıfır-maliyetli API'mize fırlatıyoruz
        EngineResult result = dispatcher->DispatchMatrixOperation(op_str, mapped_matrix);

        if (result.HasErrors()) {
            return -3; // Hata: İşlem başarısız (Hesaplama hatası)
        }

        return 0; // Kusursuz Başarı
        
    } catch (const std::exception& e) {
        std::cerr << "[AXIOM FFI FATAL] " << e.what() << '\n';
        return -2; // Hata: C++ Çekirdeği Çöktü
    } catch (...) {
        return -2; // Bilinmeyen felaket
    }
}