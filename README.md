# Nimble

Nimble, gömülü sistemler için tasarlanmış küçük, deterministik bir C++17 çerçevesidir. Zaman tetiklemeli, döngüsel (cyclic) yürütme modeliyle cihaz bileşenlerini açık bütçeler ve yaşam döngüsü kancalarıyla düzenler.

## Öne çıkan özellikler
- Deterministik döngüsel yürütücü (major/minor frame)
- Hafif, POD tabanlı `Device` tanımları (init/start/stop/update/on_fault)
- Derleme-zamanı programlanabilen çizelgeler (`nimble::MinorFrameDef`, `nimble::ScheduleDef`)
- Cihaza özgü WCET bildirimi ve bütçe uygulaması
- Dinamik bellek, istisnalar ve RTTI kullanılmaz — freestanding uyumlu

## Dokümantasyon
Projenin otomatik üretilmiş dokümantasyonu GitHub Pages üzerinde yayımlandı:

https://yusufkerman.github.io/nimble/

Bu sayfada API referansı, kullanım örnekleri ve mimari açıklamalar bulunur.

## Hızlı Başlangıç
1. Başlıkları proje kökünden dahil edin: `framework/include/`
2. Dökümantasyondaki örneklerden yola çıkarak kendi `Device` dizilerinizi ve `Schedule` tanımlarınızı oluşturun.
3. `nimble::cyclic_init(...)` ile `nimble::ExecContext` başlatın ve `nimble::cyclic_poll(&ctx)` çağırarak yürütmeyi sürdürün.

Detaylı örnekler `examples/` içinde mevcuttur.

## GitHub Pages
Dokümantasyon `gh-pages` dalından servis ediliyor. Sayfanın adresi yukarıdaki linktedir.

## Lisans
Depoda lisans bilgisi mevcut — lütfen repodaki lisans dosyasını inceleyin.

Katkılar ve sorun bildirimleri için GitHub Issues kullanabilirsiniz.
