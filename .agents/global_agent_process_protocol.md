---
title: Global Agent Process Protocol
version: 1.1
author: Fabulous-Samurai / Agent Automation
---

> **Status Update**: Phases 1 through 12 have been successfully completed, including the recent System Weakness Audit (Phase 12) which eliminated severe memory and concurrency bottlenecks. The repository is now transitioning to **Phase 13**.


# global_agent_process_protocol

Amaç
- Bu belge, repodaki otomasyon ajanları, iş akışları ve ilgili betikler için merkezi kurallar kitabıdır. Tüm işlem ve değişiklikler projenin hedefleriyle (performans, doğruluk, güvenlik, formal doğrulama, test zamanları ve bakım kolaylığı) uyumlu olmak zorundadır.

Kapsam
- `.agents/` dizini altındaki tüm iş akışları ve ajan betikleri. Sonar/SonarCloud entegrasyonu, TLA+ model kontrolleri, derleme ve performans optimizasyon işlerini kapsar.

Temel İlkeler
- Güvenlik: API anahtarları/jetonlar hiçbir zaman repository'ye düz metin olarak commitleme.
- Tekrar edilebilirlik: Her adım komut, giriş ve beklenen çıktı ile tanımlı olacak.
- Test Önceliği: Değişiklikler önce lokal veya CI testleriyle doğrulanmalı.
- Küçük Adımlar: Bir commit veya PR en fazla ~10 küçük mantıksal değişiklik içermeli.
- Dokümantasyon: Her iş akışı `Amaç`, `Önkoşullar`, `Girdi`, `Çıktı`, `Adımlar`, `Doğrulama`, `Rollback` başlıklarını içermeli.

Kurallar (Kısa)
1. Gizli Bilgiler: Agent çalıştırırken gerekli token/şifreler environment değişkenleri (`SONAR_TOKEN`, `TLC_JAR`, `CI_SIGNING_KEY`) olarak sağlanmalı.
2. Çalışma Klasörü: Tüm üretim çıktıları `output/` altına yazılmalı. Geçici dosyalar `tmp/` veya `tmp-agents/` içinde olmalı ve `.gitignore`'a eklenmeli.
3. Issue Mapping: Sonar/SonarCloud ile çalışan adımlar `output/files_with_issues.json`'i üretmeli ve bu dosya audit amaçlı saklanmalı.
4. Formal Doğrulama: TLA+ spec'leri değiştiğinde `tests/unit/test_tla_specs.py` otomatik olarak tetiklenmeli. TLC çalıştırılabilirse model check yapılmalı.
5. Performans Kopyalama: `hpc-optimize` iş akışı benchmark komutlarını ve beklenen referans değerleri `benchmarks/` dizininde saklamalı.
6. Onay ve PR Politikası: Kritik değişiklikler (core, memory, threading, allocator, daemon) için en az iki bağımsız code review şartı.
7. C++20 Compliance (Yeni): C++20 keywordlerini (örn. `module`, `concept`, `requires`, `yield`) değişken veya parametre ismi olarak kullanmak kesinlikle yasaktır ve compiler uyarısı üreten kullanımdan kaldırılmış (deprecated) operatörler (örn: `volatile` compound assignment) düzeltilmelidir.
8. Concurrency Constraints (Yeni): Tüm yüksek frekanslı işlemler (AST tree gezinme, allocator pool çekme) OS-seviyesi `std::mutex` çağrılarından arındırılmalı, bunun yerine `Spinlock` (`std::atomic_flag`) veya lock-free atomic primitive'ler kullanılmalıdır.
Workflow Şablonu (zorunlu başlıklar)
- metadata (description, author, version)
- Purpose / Amaç
- Preconditions / Önkoşullar (env var, sistem paketleri)
- Inputs / Girdiler (dosyalar, proje anahtarı vb.)
- Outputs / Çıktılar (dosya listesi, JSON paths)
- Steps / Adımlar (numaralandırılmış, kısa komut örnekleri)
- Validation / Doğrulama (beklenen komut çıktıları veya testler)
- Rollback / Geri alma (otomatik mümkün değilse manuel talimat)

Uygulama: eksik veya zayıf dokümantasyon
- Mevcut iş akışlarını gözden geçir ve yukarıdaki şablona uymayan kısımları tamamla.

Kontrol & Otomasyon
- Agentler bu protokole uyum sağlar; protokol ihlali saptanırsa PR üzerine bot yorum bırakılacak ve merge engellenecek.

Versiyonlama
- Protokol değişiklikleri `version` alanıyla takip edilir. Büyük değişiklikler (breaking) `MAJOR` arttırılmalı.

İstisnalar
- Güvenlik nedeniyle bazı testler veya benchmark verileri repoya konmayabilir; bu durum belgeye eklenip nedenleri açıklanmalı.

İletişim
- Sormak veya istisna talepleri için repo sahibi: `Fabulous-Samurai`.
