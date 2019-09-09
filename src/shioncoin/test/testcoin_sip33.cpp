
/*
 * @copyright
 *
 *  Copyright 2019 Neo Natura
 *
 *  This file is part of ShionCoin.
 *  (https://github.com/neonatura/shioncoin)
 *        
 *  ShionCoin is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version. 
 *
 *  ShionCoin is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with ShionCoin.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  @endcopyright
 */  

#include "test_shcoind.h"
#include <string>
#include <vector>
#include "wallet.h"
#include "dikey.h"
#include "txcreator.h"
#include "algobits.h"
#include "test/test_pool.h"
#include "test/test_block.h"
#include "test/test_txidx.h"

struct sip33_TestDerivation {
    std::string pubkey;
    unsigned int nChild;
};

struct sip33_TestVector {
    std::string strHexMaster;
    std::vector<sip33_TestDerivation> vDerive;

    explicit sip33_TestVector(std::string strHexMasterIn) : strHexMaster(strHexMasterIn) {}

    sip33_TestVector& operator()(std::string pubkey, unsigned int nChild) {
        vDerive.push_back(sip33_TestDerivation());
        sip33_TestDerivation &der = vDerive.back();
        der.pubkey = pubkey;
        der.nChild = nChild;
        return *this;
    }
};

static sip33_TestVector test =
  sip33_TestVector(
			"000102030405060708090a0b0c0d0e0f"
			"000102030405060708090a0b0c0d0e0f"
			"000102030405060708090a0b0c0d0e0f"
			)
    (
		 "037cc534cb1a6fbb63c13c7254105dbdfd1131bada490f83b4a3db05e2a7c0bbf9ed6e1598e46956b67df08ad6512c5b2feb508a2fc39136b275ed8b857cb7ada2326355078d3a553584ed1008857963f0c39f904706f0189e57bd3b699f9db1db82e19c5617466276c3d6f6e26b1e557cc1de3f319fd3f5fa54f3029344c6e62ecaa9ad779b2964c6de01fd4983ac8dc268c630171850419b20ee5c089828ca50d068bb16c555b081a923a6d87ef181d93e3f00cfcb646c99630136637b07287d85e7b08a18102aa5e72dac8270fff15846167ec19b8a6d144b940f694c5b11ac4e082af22feec8966916c9c84a32e87fd384009e184114a9caedaafa346b675e79e12f7c99d0f316988a71147a1c91d909be984e7a256288e1339fc8c5865a72ecbe5e8e7c840a5feae873b4d2a9455b65a7ffbd61f8b407e6a9d6d06ea20abdfa5032c5935b79afae98f256f578244f78bfb157cc8ac60d671fbab74e4bda2c4bdd4dc01628bcb52350c1baa970178b1b61804d1a0730dd099413464d089b8cf1bae2ae864d72437bac4e52b3fae1bc2954a6dc1eb6157c72f71b981811fbf9f82f251cd2f2655ea57caf2c49149c1777ffb1e4317d63afcf8e09d6bdadf4a202a2f04bee4de2f58144baa3e4805f9fda6f5f0d109c558ba02e76389cb0806f2d7a0c80aa2d16a2efa8bf4a865d35784816d78bffbb7d2327edbdca43069348c63e6c94c4e5fbc416b6fdb70a4154e7c7426a59ad975e0f2ee3118ee223261f30cab28724c66e425946b4fd30219ae85e4f7de15fb32a114998d671aa6673fb0d2aad125178ae0151c8526f9a1c06904ed1db54dacadec019eb59de70cca067b30642c34f6310c89880172bc14fbb35f1d68d8dcfba5214b92fa43a05bb08114e23a6aac8a20141efea8122d285b7db956e7481f07d41807b17bcb9b2f2d1c2dc911be0ca9adecdeb9f1e8796730992c01f4b03f3b1fe8b6c94573a4a68dd9d0b70dc54ab9634ad9eada0ccee2272a313e7715e96520854e89771341019661612588564c228672e7eac2615e7014c5acd886fb8b29407fd3416ab13cd17b0fda276faf764569270ad3dd29c53b0c5641650cdd69a0fc0a70074cdba792deda1be634a75e161554879a79a13e38b9704348036091f27778b14765676c8a7efb25981faf3ccaf28e1ebd9218532d5b9820cb0aed90fcd4ef5d68d29f3cb6304665b5fbe93dc3480476a3f2b419e218421d17882cd9ddc830b3d32f32dd4bf5cb981b12ae01c15551daac2a6cadfb7e6cdd262357e77440fd5905793cca6ae357241e3a8357ea52cc81699a6d5420bd7487a124bcddce9fd0c36bff660a4a95e4c49aa64eae8bb9ff6da293df48795074fdf4ccb5e5d0267caa0b3cfc7587cc1915953625beaccbbcdcad6b25ec5abc802a94e85b64ebda67f6c2171747dfc89e7825f4a8de5e3601c85f4840b91dcaafe5928fedf4cafe6f31802019055414f328a089654d40198a867ffac915c5aebd11b20b1e0b118e214d71a48488b45dff10972309a26a7aa877d0719cc282f170d444aa632f53bdc46741bd17f5de02025b801d48df0a6461d68c93b4474118295249a3a5b20cd76deb5b5f56d592d2d9725d61d33fea485ccc523166a34f7af5f41cc743434096666525413d513282d88e95c837d1046a8ec99a2e20cac7a57b7aa639d45b504fdbb872ddf6cc6a5c7c9be4cd93f2a78c2bbfb5ed9bb4870ab5bef50e0cc79040ce0e4a83b53ba6de6076e0e13e163f45cb85399e5f89c77e6d3a44c1b77876a5e98a81f6bf7796055d15681c9ec8a5af34d635e127dbe902e10b312c4c99d7655caa620944270dcdfdbfb1ce9921a91f763dba7509a4bbd0c11e52b4c935df91804da49f4a0c5fbb031c8c762e4f6f42395816bc9c3d99620ca5e69d74dc2b812908c9bcd5bf967bcf796dbf1d5dbb5e96d46016aa89e8bb7aeb3bd1737b9e124612f19cedf1c265f0ce21063ce4243d38b62d0932330662226e36ad700aecf2c4f3925cfc8e6117684879210a6ba64eba0a3ce01a13773a09607f6b84b6c5d798e0f5d76b3f5fabb63",
     0x80000000)
    (
		 "031676de12130e279b22e05d0526e4e6a50a2fde1e5772a5b00505b19a933b33be1b882b5ab445ac0706325f5b3ce15509fbe35d2fdfe64333179521a6437a9d8cce5fa022eb1bf3564ecbfda0048554c5d9a09035c3e8defbb55d5c2d428983ae5090ef2892fae2c9f9e3ad7f9fb0d006cb43d87501366980ac89139b8d9fd6e3fa8cb834c79d06a9146b670f136e75f165bdb0abbd7c70a46c4e6da22ae8178173416cd249718104aa57cfaf37d7a055c4ec38fdd0af479c1947aa13448ceb59507a80e9c5d7a56400a457a5718d71f4883299eb0ff9ca81a0f109dda7806d094580ffaed6bbb39dc1a1365f6906f819c2280f278e2c15327c9f02c89f1371a8678f0210e0ac366a9d24405799d344d23162507413061c9158475df86263167219c9cfb3366dfde2db793abb25c264c6e404fd04999299d0d8e8963b2b0c2cc5209ff05afeac3d40b4a98c9405bb75d0f9e0d9e57190cae62256a100edc36f32df550761285ba47c68b1e045bb1aa63eea008a62ae2733a0725fb5ca6c646fc6b363c07fe0ce940b41f8846843296efd0b613f44dbfb5585384d7402792fe24ec0975fc4e98b4c158cb4d2e15fba53b1e32a1f80994540c162aa2160bc2e03b77697f5fb7cde64dc786bfe2778f933b2d31a6be822bab57d5bc6baea58b1409b731285976fd28ef30592bce14b67902882a47350ae19d2c867a504c6839f2474d7cc98350b63d8dc54600aaf8e7ca71900590295ae230bbb34a7cf03d4875fb8dfebccc322f732564228099fa5acee524c91442dee7b055c6f2dae6de5062b1ea7509651e2fc877bc72c43285e95329fb2a3322c3999d8c72381a2f061364bd894355ef7ae0d96776c3afd6dd01a97e61c8eda548f545421eca37f428e18f01f4516dc9ac36debc06c41ab29d7756788c8c44b6320d57b5a5b6238f93a8336b80037c572b2754a04e464dfbdf290ecef27bf0815cade6ed2afbdbc3d655df36919cf75097a14c649c25f19af6a756dbb90db2de7c303f77fce3f5351f74da9f3a7ccdec5440f50836d83bdb1f5960e1284b6c237a1cc5e8d4d8849eb2ea5df2e7f8818f4980b6fe12315eb4aa1386d7d10b60a585312656676115e83b18757776d73f6c56df83a4748f5085db78fa61363cf3eabbdcd11b22f1fb21fc18836988eeff771323fc50f6d13a7d20f8ec7b851d5cc3a993b723205a10b3ee03271ea09fba8b96022c4c1cd22d58975f3082ae305313f10cb69a1efeb39318bf4caae7ca0ded0c64d9282f6f143042bb4febfea39f963a8994c9bbd8a81925c64fe6c5f83f20a4d353be204847f00182c596a2f1d21a7262b0f19b1979fff8c042626a892241f24523dfffcefa1fddf204a41391253e877160350d5572b8cd6044744d1923b9f7a44b250f3d7be0f3c1757a32d62350f5ff76c824e52a70766b742ca99eed359e78fd36b09790edd1a66eeb6a8c8535060deff545bc8b6fd5d40264e0281eae7fdcd5b711eba0ca7efa7cc1fcc657ed97ed917da0a1f830b9bb719787ed65d2a71d9149d94f9bee7f2e23a4943a3f79bcc00323648c0738da35f0f068a0bc62e6e5e9978dca68db427916a40d40bb5d7a19721fc2af45c40043b152ce7bd8e1ccb70cb797be9878579cdb9e865f98ce862c5693d9526f9e5ef57b697c9571b64aaa75ac4ffde5f3ae3a1dd9318ad23915385a7a584eddd3b5724f17897eec101579c090f4cccc2c5fef86600d828d51af1d939c41d4921af64e90ee4abfdc4b2b0643a55985453a361d3a25f18d1326c5b2a82e5642b4f1a7e4695d087a9cd9ac85ed95a7576ae3ac2c60e8fa9219025b451337b976b824ce76249f72b252278accb3caa94576ad753401d55a6ec5ddbd07cebd316b8c935a4c3fa065ac448cf404cc9fbdee08d8366607e51c3613a05d9b24ed925df16401346949da1e8940f939d2fac7e7053e7a1e4e36ea069c1fbb34f17fbad5a26943ed1788a17b9522d8e9fdc6deef3bf8da25810a2d3b6059fac00c53a2504dbc6994c6ec2a23a73ff8a711fda670ea30e09b8c2da195ecf6ad0a622a9c44d35e3fc6f2672",
     1)
    (
			"0358a021650aff1fdf734659cc43fe10a6f26a86493104448c566a1fc2ded8ac759cf0639398b5a4fc0b32e78dbaa2c3b6befb6e28524d5934af2905cc1eaae897cbb31cc85a95b755013e4e42770f8349a9c73172c56ece5db7f4b99312deff235a3b5af8ecbacd43cf8958846a7ab1530942ece92c62903e7385fcd015a808910a595101c0835d880b728fa02b3ee7b586b5de260edbb53f0bf0ed0501ba8c087f8769347e00b9ddf1dac807415f03a87edc7c860eb7633a91e9759c9e8df6bbb77c6eb90033695c2ea730755362104df1d20e7e84e75d118caf169784f40d8aa2d3851aea155c3c7450858e9af8a4905bbd2a40493fc277d3fdd740c03aa73e0e9151cc6ac18e04476e75191919a05646e060c610eb8d3908f423cc2831b04daae0bfbb46a460b30acfc8b9705f19b5eb7743b4c346ade7c918d944c2a9424948bee84afaa6c0569417721931f8edeeb28b9aec8ef362607baaf1e4e4b66a6a9c4b778615f08a657f53aecf5538338e2bbe013fa2edf84ee29ec155a386c8ddc46e3a9100152379656959931058d7d605a51ef1feb65289be446c80aba02b994d4a9ef44151aaeed9f3f97dc5632a5bd06f7b8a27dde57c195823b2e04ed258ff73126408ebb7eed727bac3e12d617670af164b53bc4761a8f0233a7c9c3b62a291e1ce1e97b081f209cb645e6e600789f33e333cedbeb0c1c136f65563cf69f806e488996f4151ef2d3bf22f70247c00b37615de5877c03677fd15eff2822e6248c336361617b86a8edacb344d9e0a0a2af9e05259a98c16f6ed33429b0897b4a322f2695cb18bd7329104c0788a258973076c923325b8ea176bc78f384bb7b07206a9a6c0205d45ee21c835148173ffb9103eadd96e946f0388aa8b957d9c315c7f1614378f7369454f9d65a5d38622b72ddd9f34f19bbc74b8100465bc73199e9284827b24e9c45e2f06a2b5140f9d95e8f72a8b475fa2caac649a918a03dc0facc9cc0522b4103a7e1f6b116cab8abfc42b4d753671258a110498f8426e6c2162e8c25e6e28575f7315df0da89f576e9377333aa851e0baa7be4711d8f24fdb7ec67ad7b7e4578a8435329d284a1259dda90f8c541dc14b3e7ec461be7c8ca324d5e14f017c2f411bf5327b87efd0bc1a8d312558e4b6469d508d437ae8a7352ba05d5d748be040c4cf66d5f29f1c98374db66cf161fd762b8ddffe8016aa43dfa06899b85b79d3eb6c21a973509454f3aa98dc346edeccd3be4ba61d49b02dc7c092b35cf48581bafa8c011296b94736d2a1679ea6c6fcca60338e8e40feb8e5bbfdbfe19a84be7fcacda74b882bc4011390139fb76e49701ec8b6c516bcca2923e3617497383b2c594495e526f03521c7176d4f5c7dce9ad85294d23d7ad77798d57fc4a8d003bc748c25c86b2e0d2073b0e01faa2558331dbccad2ca1a709bf61a53fb55fcd03dd8e1778108bb0bb9d849f5296f21d378a2cb86cea170780a4baff97fa550cf28e06a8206e856103d490da02445f6d5d489e9b49489327a4fd1e8176d78077d5f3844cc49d98f12ee58f363c7cf8f491f1b2456028acf62687f05bfbf12685634f20009eebd302969b3a56d04795514b69990b4c12d30a8471aebd35d2dc4c360267cf996b4bbffe6489511e924db5f2fb0ca803d4c5de5298b599f39d1146082cd1783e3c79d6532b12fe04a565a4f1bca90ea9ed4acd328e798b7b10e01ee04161f386ec411e75ebf912bab9b640e39b05c916ee1bc726b95128941fb557be88fb91e7d0287a304bd170b256e58655989727c8d29cc353a0c4e83accb1e8aab54613892c56f1889aba480337736684e79537ef055d93a40d4aba3c4910fbb9a72bca8334607d6823cfba127064bf22eacafe643ceaf248882094529691a23a2932ca403917892cf1816080ad83d5d8d21d36f215085fcd2d4b3c83fb8f3467d5ca19a6a9335f7f628b2b2dbed412cf3728c43da22ec1822db39b8c17e9b8e37c9e3a27e48981515e2f0a7ba502d76defeac1db02ddc5c85a17a6ad87e4bb9fd0f4d0b249fb5b222dfba412856bcb0683fb1e0592b",
     0x80000002)
    (
		 "033d907d61e9919a04aff3a8004071b71c9c3aa4fae32796acf97e10be1f62dd4c0131c5a8d44963d34252dc1e4e3adad9752fb2c4f8059856a3dabed111d0654d17a1beb3d2962d8e38cd5025e5ded36a135cd0e3457444b8473eccf3d8f145aad616ac63822c47d743d8f5529fc59e9b1b01183e2f62c70a5255366b4dba6d000d37919d3fa4d6a2ef2ca5841c1ae8ef2d21afddce5b67b62dd1f28ba4b7568f22f01820b15960463a0a14526d57b4f05531171a7fde231f7693c6c9e58f50e252b92d68f0d5a3e83b7784b5a8d01404b54b9735c52f417d4fac260033c84687de4133634ec87fa4cc8d54b17273f98115568ac19e366a347d6286134341e012f22157ca65da132bcde3a273403e8b78655eb067f0d2cc710b768b35614a7b3b9766a163096e63cac83448f8147fec4e3989557c08187cf55d126b9d56c1f28c6a6b8c14d2aa724970310783eda40eafdf562856171e48f5ec61cf4d47474eb9d051c19e26045748b78fd5b02651ab1ebd32480d0139bff1a03017aa308ce7e1e74685343d46eb8cbf11873a60f54665433f694c0cf5bdfc7728ae1d940a4e1cedb08a791aebb898d4ead457545ef12a0464bdbadc7704b3377c75a3b45ed9186085564919b1768d337f74331c16d3830c96e6f0c0962e2a1e1c369e019539baa30c90e7088a8f3c82e6fc930cd82d52b1d1a2d227375feb2ce35b0c4cb11110f4708b1f082db1fa09746fea7831e719ab73ca4d730cf2f04ad20fc85fb773aa45644669ee144d6d36fb482af99c24b728dcfce587d7bf1c41bda6c2d7ed32b8860182647ac5bedd9d96d7a7078fe99372407228807041e4c7d982441c88afedb51dcead6e150e758da4395a8f5e9005f4eef075b4a096c8611f0e062a78ce2cf13f5a34bf12897ed8aceec6f086b73632461e228c3531d10f9e3f752edef1ab28ec07238dfb7c127ac1da25edc26b240325aad169bf69216d7eb82eac7331e5582823183ee3c01c5ea94a1aacd0a19d0befa924d8df34f3a09170447399596571a735145a984872e9ec73dfbd730ad089c8a9cde212063de831165398ce4e7f49739869ed327c9a420c5e5a775f5691ddc76f74d67d9de2a13685524a01d7e49ca9e1b78681e288219da7096d2dcbc9b90983ef3167c2c0c04680e3c108947bc12f55139f3db5f5337b0c718e6099fe2504fb9580ccf16280df66705a7f232101fcebdcaf9e2c16bb3cf7cdf7c1aa06b165352f78bd01dbbb948ce1b2cbcca0af22ea0bde79cd5887bc164e1d8cf3a20937738d07caf6c3907985f7937af75520d4e6cccaa329ca4c8529b87855cd4c31b5417bc8367bdc3fe77c05809f891cb9dd13a276abae3088ef97a94af1f3510220408b0ac802a6ccff08ff7bbdfae7f4d265e6cee8ca5003e3d004d489343bd30c167ed03b4588fc47da58f5664ac023c6cb7180b965453bc523f072050594920275795046745ecb2fa06bc8f90507a49488a998d09a461d6407c5c4aa940e799e6c56fb86877c7e17a0c615c66596fe512cc4cdf9bd2014076c72fc48b99f95c43a6a2e10aca531e517b3141c14b418ed2ec9364822f842d44a8d8c7741ad5367265fe143cc5fe8f669f1c60a67570856a0ff04dc3612c6306e5a2e984d42e458a4696ccb365c7e0028963e53e5d1359fa17a51a50217911ab78cc39c66c53b9f31c8600811dc52da09c2e782969ec804e6feda296883ba2077eec33d9e0bb80df7494ce78ac38092196b86b228d212fd34bc92208c93597f153e512da8f4ecf734c88aa084c2c4a1a4b0d5eeaa95bf175ed24c4cc20f15cdbce4464272023024c97d2641f77c7d6c63efd53407c3fee036d1d3c1b23cdc313417b1a015ecb79273c5e098f8571b577c527b01a4a6c9112a4c2276dbc137c4fb4e807a88d59d6d0fd1ddd4f75ed06bf70901089d621dfa0ac1ca347ac472e858b0df18b1bb4fa269af019842be683e258cdba31a65f669218f6ae3e9a8912dd30ba6793ed714bcb05883c7bb8d286239bb609af4f0b8b15fabd35525a92bcc18d753794edf12d3ac8b72caca1bdceb89852b2481f3",
     2)
    (
		 "0379f1ed4158a0e882589a62165beb6dd710551c6de7c21d3d1139465ac0562aa3d6f79130ba52235b4c911357c8764d00b04b5af14f013cd00f9c441c6c6932506f229e8f4ea581409fd674b61e3cd9f0e785919cd476d0fedf35fb0ac0e0e64e86cd748dc582cbf633b5b04866a25f9b62cc1cf62e6cf8dc408e7ba886d7290a58e46e73d8d61a88f8c50465edca9b2a15a0c560dc2a9764ddd6448aba76fa3caa60837049634847a585cfab6e7a17f0426ebc5a71810c31dd33230b216a9fdfc2925d66440ce02b2f1860fc603ec370c4f596311cb3a43e1c3d5d1de75120321f06d90d24c7af9dbcc0b2b0aff1d480124f6ceafa1bdd2e61be3664ee4e1e7067eb1d03b2cab1db6426185995774e2d116c2fd8699e77116f0946e5012a8f269264cb25c07f3a16169d271dc59f61928daa8cc2d9dcf60097cc4616133586c0413245575b4e967ab1b5f9b82fa795ac74029edfb07a88449701ccc8b27d88852dbd1661ccad78ed435cd1873df3a4248eb63008e41dbeb63bebf5123d76aeb00775721d322a634e69eeb88346f6abfcbf9aab8d2738f0c94af1366de2f1eac81038b5aece16fe0302a7b0616d0271ea3be312a8ca2238a1aa13f7e1dd674b27d38a58c6d93d422a6feff71c3481d599e66ef79167cbdd30cc9829745597b204898e884cbd05b8929a67dc022967cfec74ba85f472129919efb213b1aaee24694b5a2b2e2a0d421d3363afb17595a70766be0a7b46d95cc8a0e8efad8dcc4bdb2d756b948669f5a597d2954985b3d40b02ba940bb8d58256106c6777e34004cecd22fa52dc77ba80a7e5281c8cd57072373e8c3d8b873bb164d0650abec4ec99acd7ce006ab6d362a93a804ee7cfbbbde71c4449288fbc20b14e03b3d9396a9a63b5361d2e138e4ef57232cd0e770002abb3d6739ac5eb07bbc3e373bf56d9a544f6f440c24d8edabca9d5d3626734500026f02b599c9012991eb609813ea2cab387596f30be8f6ad15cc01d0209718c2534e0f6a1dd1bce9c065a337834f861348da4e12b05f5b28d408800666ff1c683e00bf8682fbd9ceaa71269fff9b33406c866015bd14a5166725858e710f260b2b11142c679549252d2e8a6540f437c74fbb6dbd18275d7c9287880e33ee08653aa08525ff720c8d5b0746928cc4c18aae53e1dcab028aa6831f177988c7663eb9e1d24eaca5f47a4c6a83dda5ea630de608ab61302fe2653fbee6e6e07a721ef92aad7c5270154f76a3e1f82ab4f5d0e799438547710bcc88f12a2f24ded007a72dcef62e8fbb646f60e18c38691002903faff61b885a993c79a122fbeeb3c8f7b1d7c7574d169bade8040d000ad95d2abb026448771c12e8f6ff913fddf0fac2030f553979b48435e74e961b18a61c23f5267570c8564e0482210f4cc11c193a421db80ad5a87c13970224623234ea8284820234d147ac9424f0c73d02a891c87d5b22003bcce94e2d2b228eca29cf850404b90753edccc13f649dea00511f76defb72a73a91571fdfd167f6ecbe07a7561af311c05469974381b4bfc3efcf44b3b1f120e8833dc6eb13e07046a2228bcb0d5946ca3355c7083a2292d6a5e082742c1b4d95fd676f923a828fb8aefa447fc736a05282d6d727f85589f760763cc8f687e77073cd64af35eab29dc67d87410da31052d67c293c1e4923885bb4b8921c4c17273fdbd4b53762c23a764553d0d746d2b355fa411ed8659f3c221668d321d1c85b99d935e725b107d93367cc896a14c01c34da8759fc9188da33d8e46177ac3b11b14ad087f7b57bc5f29cbc325a90a17d68dbf4e4814115693393ff7dd1f166245fafe222e06bb0787a253f32da6489da7b8b3bcd99da3e96348a2e7b2de93c6663ba694dfe524b542d63277e65c77abbd65c95824a08deb1a4513a9696dee223966b89badc86d471f51a071cca33b0c37b9fb40c0e8142684021aac2fbbf9e921ccd00833c5335430ed77f8b46f275629c1a6ad6c58e359b2fc5a165af8c43a0fa2e12ba0548ab18035d32786e0307eb365fbd97abaf6e962edccc02c3a7cae56f47f6ee6caf064ba51",
     1000000000)
    (
		 "03e6e6cd1c50983576891b666263ed6e03c45b7bf771f8f5a2919405baaee1b0364c670e758c215372cc6dca05a28974a31f32c0944a701abc4cddaf8d780ebd3a680f3d25e2eff462fb425c22813c7d8d040bd717cfc4b4e27acf9b7cf5e522f257467b5a3e0ab7de39d1c6e3ab5cdb380f6b2c56b3837744653f6be4ee60145b0970991b5f5e48352862ff3690c1292d0f020842aa1ed4a520ce07867783d3872f9c41672c0bc0479d555f41564d8482628c8a294a50693cf203fcda3b07ac78253b6c87ff10f3298cad6065e96582a7082e4f0d1a99a91909023b6f5fae5d9c770d45cbb22fcbd86556b7f4e202caf3bc551e9602d50aefc7283711be172a95080ae58f16e624c81c21c865ad075b187c1209aad8455e929641e1b87b5f2eeba1a7567814f143eb0d08e4cd124cc4b4e4f2df87d1475188e6508e1d255bd5650a5ab9418981102f50a48c53d2203377d62e3913b9f5922ba7d409761ecde247b94ee70b0ac5adec3c429544ed660de116d203eb93c808be6187e64536b63ef28528a1600389217bcd95e6341eaca2aca212293e2e4f0ae4d1b0394168068be9a8cca632739d8a7a0cd5d1660cfe0816186c257083984b2eb67f7e18754aba0adf156853fa78be58916abf79570fef4214afedd5eb2f83e9d571cef8ab4c11020dc5132d71394dc4f060bafcdea03f50615a08f7b505d45be6954e395d6a7c7df5070c56e8454d7da1c2381d33ee81fcbf2f94ab0fc61435b297798f54128a09f0fa5a3700ff44167fa92eadb734e1ed706f47866c97ee5e59558f16f8471d7e6cdf732be48561f1cedbf8d9a427338db2befe1a9f9a3abe2dd62084881d262b9e3f81b7bca2efeb1728a3636cf6afe56c1983a315abcb9658a65727e99a9ceae5ec7d3516918919b0f1571abed248736f6a848b47d5c5da9f3dbbbb44e96dd8de6e36e975f9e5dbd3d7d63aeb8a4c7b41ae799b2eb74ed388ae1be7ced7971b959947dc5151f44692a147ccb15b678abff5596eeec6c7d9777c5bfd52bb6edc4fc84c029e110fd119a5f66a7917fc371e2f8917f87efc3dfbf9885f56178f0a73358b7e5f6542d1d33bb24d74935a977c7f188d79aaf62b2a61ff12caf099c0d8444ed42adda432b182b34ceeabf5c93cfc7c28381960eff08efa176a0acf851e0fa5267f19552b4ca79e5cefe6400b96b81bf15c1b1f9114a18c15f01f11b37217d4a704b26c5d74dad474d5a74bdf8d49b44365e291801711f07112f0c50b95e84908cd44d4e4258532fd9d01998ae2f53254eab59593f210fafcf88c9c149624bfbc15459a058ee898816c47ce99978070bffe19641b97768852128fcd7a264fee99631474547ea062eab5ca12e5d4d77138cf76a0670965f128c49cc046a0d8325108d0e174361c95df72d4bf3512c3336ef468f0a36fe8e283c185fa886f25f6b99c9e5900eed7ccdecdf94c2357292a01a9e6a46149f45f2a56dbe0a275307837f76b79921c01474ef810435e0757e87be762960f80c294b165c01e5f542fb02f8ec6e4d0be388988552e2196c77985ab8ef9b6ced721b62cdacec4c3e0965986b9a607fca5cb333dfca6e5499ba4b124a6066c2ce76b7f7b14c0535aa2446108ab84eca184dd7df6391911b1699d3e3655561d79aa44fa8ae4f06920dd3633e8c7d516f194acde09507ff0080c65dc1f00d3c22700cd0ece0b709f5193fa5b08648820c003a280ce54f4228cddb878ca839f594f85ec798c22dc3adc57c1f02bbaf199c80de9f6c9504de46a4ef428b790abf5d9dca92f7e37220d8d14ce94fcc562e6407f442348608cfce9808048feff0efb32a51051ce7a4bdedc9a4cdc70f474e306a3d049cc10baa06d39e4247ecc94eca27e7e3bb5c1e2dad4d9df9d46a4f046e4190bda6603eba4b734c8c0695fc0afcd1c47f191341ab414d2ca4267465038e8d327e33cfc1707cceba797b989d31b3a5defc54c73dbc7bdf73369a6afb726c041c5181ad4ee224a96fb5b2d6b60fa7a323d235ac9d4ac32ec13137e8c0f7f60ece1c3fd6891c14377ada4eb537df90427edbbcf6ec08550",
     0);

enum Base58Type {
	PUBKEY_ADDRESS,
	SCRIPT_ADDRESS,
	SCRIPT_ADDRESS2,
	SECRET_KEY,
	EXT_PUBLIC_KEY,
	EXT_SECRET_KEY,

	MAX_BASE58_TYPES
};

static std::vector<unsigned char> base58Prefixes[MAX_BASE58_TYPES];


static const std::vector<unsigned char>& Base58Prefix(Base58Type type) 
{ 

#if 0
	base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
	base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
	base58Prefixes[SCRIPT_ADDRESS2] = std::vector<unsigned char>(1,58);
	base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
	base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
	base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};
#endif
	base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,48);
	base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,5);
	base58Prefixes[SCRIPT_ADDRESS2] = std::vector<unsigned char>(1,50);
	base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,176);
	base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
	base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};


	return base58Prefixes[type]; 
}

#if 0

template<typename K, int Size, Base58Type Type> class CBitcoinExtKeyBase : public CBase58Data
{
	public:
		void SetKey(const K &key) {

			unsigned char vch[Size];
			key.Encode(vch);
			SetData(Base58Prefix(Type), vch, vch+Size);
		}

		K GetKey() {
			K ret;
			if (vchData.size() == Size) {
				// If base58 encoded data does not hold an ext key, return a !IsValid() key
				ret.Decode(vchData.data());
			}
			return ret;
		}

		CBitcoinExtKeyBase(const K &key) {
			SetKey(key);
		}

		CBitcoinExtKeyBase(const std::string& strBase58c) {
			SetString(strBase58c.c_str(), Base58Prefix(Type).size());
		}

		CBitcoinExtKeyBase() {}
};

typedef CBitcoinExtKeyBase<DIExtKey, BIP32_EXTKEY_SIZE, EXT_SECRET_KEY> CBitcoinExtKey;
typedef CBitcoinExtKeyBase<DIExtPubKey, BIP32_EXTKEY_SIZE, EXT_PUBLIC_KEY> CBitcoinExtPubKey;
#endif


#ifdef __cplusplus
extern "C" {
#endif

_TEST(sip33_hdkey)
{
	std::vector<unsigned char> seed = ParseHex(test.strHexMaster);
	DIExtKey key;
	DIExtPubKey pubkey;
	key.SetMaster(seed.data(), seed.size());
	pubkey = key.Neuter();
	uint256 hSign = 0;

	for (const sip33_TestDerivation &derive : test.vDerive) {
		DIKey& dikey = key.key;
		const CPubKey& dipubkey = dikey.GetPubKey();
		_TRUE(pubkey.pubkey == dipubkey); 

		const cbuff& pubkey_raw = dipubkey.Raw();
		const cbuff& cmp_pubkey_raw = ParseHex(derive.pubkey);
		bool fOk = (pubkey_raw == cmp_pubkey_raw);
		_TRUE(pubkey_raw == cmp_pubkey_raw);

		{
			cbuff vchSig;
			_TRUE(dikey.Sign(hSign, vchSig));
			_TRUE(dikey.Verify(hSign, vchSig));

			DIKey dipubskey2;
			_TRUE(dipubskey2.SetPubKey(dikey.GetPubKey()));
			_TRUE(dipubskey2.Verify(hSign, vchSig));

			DIKey dipubskey;
			_TRUE(dipubskey.SetPubKey(pubkey.pubkey));
			_TRUE(dipubskey.Verify(hSign, vchSig));

			_TRUE(dikey.SignCompact(hSign, vchSig));
			_TRUE(dikey.VerifyCompact(hSign, vchSig));
		}

#if 0
		unsigned char data[74];
		key.Encode(data);
		pubkey.Encode(data);

		// Test private key
//		CBitcoinExtKey b58key; b58key.SetKey(key);
//		_TRUE(b58key.ToString() == derive.prv);

//		CBitcoinExtKey b58keyDecodeCheck(derive.prv);
		DIExtKey checkKey = b58keyDecodeCheck.GetKey();
		_TRUE(checkKey == key);
		//        assert(checkKey == key); //ensure a base58 decoded key also matche


		// Test public key
		CBitcoinExtPubKey b58pubkey; b58pubkey.SetKey(pubkey);
		_TRUE(b58pubkey.ToString() == derive.pub);

		CBitcoinExtPubKey b58PubkeyDecodeCheck(derive.pub);
		DIExtPubKey checkPubKey = b58PubkeyDecodeCheck.GetKey();
		_TRUE(checkPubKey == pubkey);
		//        assert(checkPubKey == pubkey); //ensure a base58 decoded pubkey also matches

		CDataStream ssKey(SER_DISK, CLIENT_VERSION);
		ssKey.read(
#endif


		// Derive new keys
		DIExtKey keyNew;
		_TRUE(key.Derive(keyNew, derive.nChild) == true);
		DIExtPubKey pubkeyNew = keyNew.Neuter();
#if 0
		if (!(derive.nChild & 0x80000000)) {
			// Compare with public derivation
			DIExtPubKey pubkeyNew2;
			_TRUE(pubkey.Derive(pubkeyNew2, derive.nChild) == true);
			_TRUE(pubkeyNew == pubkeyNew2);
		}
#endif
		key = keyNew;
		pubkey = pubkeyNew;

		CDataStream ssPub(SER_DISK, CLIENT_VERSION);
		ssPub << pubkeyNew;
		_TRUE(ssPub.size() == 1517);

		CDataStream ssPriv(SER_DISK, CLIENT_VERSION);
		ssPriv << keyNew;
		_TRUE(ssPriv.size() == 172);

		DIExtPubKey pubCheck;
		DIExtKey privCheck;
		ssPub >> pubCheck;
		ssPriv >> privCheck;

		_TRUE(pubCheck == pubkeyNew);
		_TRUE(privCheck == keyNew);

	}
}

_TEST(sip33_tx)
{
	CWallet *wallet = GetWallet(TEST_COIN_IFACE);

	{
		CBlock *block = test_GenerateBlock();
		_TRUEPTR(block);
		_TRUE(ProcessBlock(NULL, block) == true);
		delete block;
	}

	DIKey key;
	const char *secret =
		"000102030405060708090a0b0c0d0e0f"
		"000102030405060708090a0b0c0d0e0f"
		"000102030405060708090a0b0c0d0e0f";
	key.SetSecret(CSecret(secret, secret + 96));
	wallet->AddKey(key);

	const CPubKey& pubkey = key.GetPubKey();
	CKeyID keyid = pubkey.GetID();

	CScript basescript = GetScriptForDestination(keyid);
	wallet->AddCScript(basescript);
	CScript witscript = GetScriptForWitness(basescript);
	wallet->AddCScript(witscript);

	WitnessV14KeyHash di_keyid(keyid);
	wallet->SetAddressBookName(di_keyid, "");

	/* verify spend of generated DI3 transaction. */
	CTxCreator spend_wtx(wallet, "");
	//  _TRUE(spend_wtx.AddInput(&gen_wtx, nOut));
	_TRUE(spend_wtx.AddOutput(di_keyid, (int64)COIN));
	bool fSend = spend_wtx.Send();
	_TRUE(fSend == true);

	/* verify insertion */
	for (int i = 0; i < 2; i++) {
		CBlock *block = test_GenerateBlock();
		_TRUEPTR(block);
		_TRUE(ProcessBlock(NULL, block) == true);
		delete block;
	}
	_TRUE(spend_wtx.IsInMemoryPool(TEST_COIN_IFACE) == false);

}

#ifdef __cplusplus
}
#endif
