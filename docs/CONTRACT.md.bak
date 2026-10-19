# Kontrak Antarmuka CLI aesterctl (CONTRACT_VERSION = 1)

Binary dijalankan via root:
su -c /data/adb/modules/aester/bin/aesterctl <subperintah>

1. Perintah dan Format Keluaran:
- aesterctl version: "<versi modul> <contract version>" (contoh: 1.0.0 1)
- aesterctl cpufreq get: persentase limit yang aktif (contoh: 70 atau off)
- aesterctl cpufreq set <40|50|60|70|80|90|off>: nilai yang berhasil diterapkan
- aesterctl governor list: satu governor per baris (irisan semua policy)
- aesterctl governor get: nama governor aktif atau "mixed"
- aesterctl governor set <nama>: nama governor yang berhasil diterapkan
- aesterctl io list: satu scheduler per baris (irisan semua block device)
- aesterctl io get: nama scheduler aktif atau "mixed"
- aesterctl io set <nama>: nama scheduler yang berhasil diterapkan
- aesterctl apply-saved: menerapkan seluruh konfigurasi tersimpan
- aesterctl status: key=value kondisi requested vs actual

2. Exit Codes:
0: Ok, 1: General error, 2: Invalid args, 3: Not supported / belum diimplementasi, 4: Write/Verify failed
