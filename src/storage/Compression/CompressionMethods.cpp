
#include <span>
#include <array>

#include "Compress.h"


void compression::encode_ts_price(std::span<const uint64_t> data, std::vector<int64_t>& arr, size_t& used) {
   
   arr[0] = data[0];//first timestamp needed to calc subsequent timestamps 
   
   if(data.size() == 1) {
     return;
   }

   used = 0;

   //delta of delta
   for(size_t i {2};i < data.size(); i++) {
      arr[i] = (data[i] - data[i - 1]) - (data[i - 1] - data[i - 2]);
   }

  arr[1] = data[1] - data[0];//first delta

}

void compression::encode_vol(std::span<const uint32_t> data, std::vector<int32_t>& arr, size_t& used) {
   
   used = 0;
   arr[0] = data[0];
   
   if(data.size() == 1) {
      return;
   }

   for(size_t i {1};i < data.size();i++) {
      arr[i] = data[i] - data[i - 1];
   }

}

void compression::write_ts_price_vol(BitWriter& writer, const std::span<const int64_t> dods) {

   if(dods.empty()) return;

   //single element blocks still need their first value
   writer.write(dods[0], 64);
   if(dods.size() == 1) return;
   //delta or delta
   writer.write(dods[1], 64);

   //https://gist.github.com/mfuerstenau/ba870a29e16536fdbaba
   //>> 63 smears the sign bit across all 64 bits
   auto zigzag = [](int64_t val) -> uint64_t {
      return (static_cast<uint64_t>(val) << 1) ^ static_cast<uint64_t>(val >> 63);
   };
  
   //start at two to skip header
   for(size_t i {2};i < dods.size();i++) {
      const int64_t dod = dods[i];

      /*
         This is the control prefix, it tells us how much bits to write based off our value
         When compressed data is read, we will scan for a control prefix,
         if for example, we find '10' we know to read 7 bits
      */
      if(dod == 0) {
         writer.write(0b0, 1);
      }else if(dod > -64 && dod < 64) {
         writer.write(0b01, 2);
         writer.write(zigzag(dod), 7);
      }else if(dod > -256 && dod < 256) {
         writer.write(0b011, 3);
         writer.write(zigzag(dod), 9);
      }else if(dod > -1024 && dod < 1024) {
         writer.write(0b0111, 4);
         writer.write(zigzag(dod), 11);
      }else {
         //64 bits so large dods (e.g. ns timestamps) aren't truncated
         writer.write(0b1111, 4);
         writer.write(zigzag(dod), 64);
      }

   }

}

void compression::write_vol(BitWriter& writer, const std::span<const int32_t> dods) {
  
  
   if(dods.empty()) return;

   //single element blocks still need their first value
   writer.write(static_cast<uint32_t>(dods[0]), 32);
   if(dods.size() == 1) return;
   writer.write(static_cast<uint32_t>(dods[1]), 32);

   //https://gist.github.com/mfuerstenau/ba870a29e16536fdbaba
   //>> 31 smears the sign bit across all 32 bits
   auto zigzag = [](int32_t val) -> uint32_t {
      //arithmetic right shift to make negative/non negative btis fill with 1, 0 respectively
      return (static_cast<uint32_t>(val) << 1) ^ static_cast<uint32_t>(val >> 31);
   };
  
   //start at two to skip header
   for(size_t i {2};i < dods.size();i++) {
      const int32_t dod = dods[i];

      /*
         This is the control prefix, it tells us how much bits to write based off our value
         When compressed data is read, we will scan for a control prefix,
         if for example, we find '01' we know to read 7 bits
      */
      if(dod == 0) {
         writer.write(0b0, 1);
      }else if(dod > -64 && dod < 64) {
         writer.write(0b01, 2);
         writer.write(zigzag(dod), 7);
      }else if(dod > -256 && dod < 256) {
         writer.write(0b011, 3);
         writer.write(zigzag(dod), 9);
      }else if(dod > -1024 && dod < 1024) {
         writer.write(0b0111, 4);
         writer.write(zigzag(dod), 11);
      }else {
         writer.write(0b1111, 4);
         writer.write(zigzag(dod), 32);
      }

   }

}

void compression::read_ts_price_vol(BitReader& reader, std::span<int64_t> dods) {

   if(dods.empty()) return;

   dods[0] = static_cast<int64_t>(reader.read(64));
   if(dods.size() == 1) return;
   dods[1] = static_cast<int64_t>(reader.read(64));

   auto unzigzag = [](uint64_t val) -> int64_t {
      return static_cast<int64_t>((val >> 1) ^ -(val & 1));
   };

   //bits to read for each prefix, indexed by how many 1s it has
   //0 -> 0, 10 -> 7, 110 -> 9, 1110 -> 11, 1111 -> 64
   constexpr std::array<uint32_t, 5> widths {0, 7, 9, 11, 64};

   for(size_t i {2};i < dods.size();i++) {

      //count 1s until a 0, the longest prefix is four 1s
      uint32_t ones {0};
      while(ones < 4 && reader.read(1) == 1) {
         ones++;
      }

      //width 0 reads nothing so a dod of 0 comes out as 0
      dods[i] = unzigzag(reader.read(widths[ones]));
   }

}

void compression::read_vol(BitReader& reader, std::span<int32_t> dods) {

   if(dods.empty()) return;

   dods[0] = static_cast<int32_t>(reader.read(32));
   if(dods.size() == 1) return;
   dods[1] = static_cast<int32_t>(reader.read(32));

   //https://gist.github.com/mfuerstenau/ba870a29e16536fdbaba
   auto unzigzag = [](uint32_t val) -> int32_t {
      return static_cast<int32_t>((val >> 1) ^ -(val & 1));
   };

   //this undoes write_vol() and write_ts_price_vol 
   //same as read_ts_price_vol but the last width is 32
   constexpr std::array<uint32_t, 5> widths {0, 7, 9, 11, 32};

   //checks how much ones then reads mapped amount based of ones
   for(size_t i {2};i < dods.size();i++) {

      uint32_t ones {0};
      while(ones < 4 && reader.read(1) == 1) {
         ones++;
      }

      dods[i] = unzigzag(static_cast<uint32_t>(reader.read(widths[ones])));
   }

}

void compression::decode_ts_price(std::span<const int64_t> arr, std::span<uint64_t> data) {

   if(arr.empty()) return;

   data[0] = arr[0];
   if(arr.size() == 1) return;

   //delta is rebuilt as we go, then added onto the previous value
   uint64_t delta = arr[1];
   data[1] = data[0] + delta;

   for(size_t i {2};i < arr.size();i++) {
      delta += arr[i];
      data[i] = data[i - 1] + delta;
   }

}

void compression::decode_vol(std::span<const int32_t> arr, std::span<uint32_t> data) {

   if(arr.empty()) return;

   data[0] = arr[0];

   for(size_t i {1};i < arr.size();i++) {
      data[i] = data[i - 1] + arr[i];
   }

}
