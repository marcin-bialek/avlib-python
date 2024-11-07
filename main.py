import build.avlib as av
import numpy as np
import keras
from keras.api.layers import Input, Rescaling, Conv2D, Conv2DTranspose, Concatenate, LeakyReLU, Activation
from keras.api.models import Model


def make_model():
  m_in = Input(shape=(720, 1280, 3), dtype=np.uint8)
  m_resc0 = Rescaling(scale=1.0/255.0)(m_in)
  # (720, 1280, 3)
  m_conv0 = Conv2D(32, kernel_size=3, strides=1, padding="same")(m_resc0)
  m_act0 = LeakyReLU(0.2)(m_conv0)
  # (720, 1280, 32)
  m_conv1 = Conv2D(64, kernel_size=3, strides=2, padding="same")(m_act0)
  m_act1 = LeakyReLU(0.2)(m_conv1)
  # (360, 640, 64)
  m_conv2 = Conv2D(64, kernel_size=3, strides=2, padding="same")(m_act1)
  m_act2 = LeakyReLU(0.2)(m_conv2)
  # (180, 320, 64)
  m_conv3 = Conv2D(128, kernel_size=3, strides=2, padding="same")(m_act2)
  m_act3 = LeakyReLU(0.2)(m_conv3)
  # (90, 160, 128)
  m_conv4 = Conv2D(256, kernel_size=3, strides=2, padding="same")(m_conv3)
  m_act4 = LeakyReLU(0.2)(m_conv4)
  # (45, 80, 256)
  m_conv5 = Conv2DTranspose(128, kernel_size=3, strides=2, padding="same")(m_act4)
  m_act5 = LeakyReLU(0.2)(m_conv5)
  # (90, 160, 128)
  m_cat0 = Concatenate()([m_act5, m_act3])
  m_conv6 = Conv2DTranspose(64, kernel_size=3, strides=2, padding="same")(m_cat0)
  m_act6 = LeakyReLU(0.2)(m_conv6)
  # (180, 320, 64)
  m_cat1 = Concatenate()([m_act6, m_act2])
  m_conv7 = Conv2DTranspose(64, kernel_size=3, strides=2, padding="same")(m_cat1)
  m_act7 = LeakyReLU(0.2)(m_conv7)
  # (360, 640, 64)
  m_cat2 = Concatenate()([m_act7, m_act1])
  m_conv8 = Conv2DTranspose(32, kernel_size=3, strides=2, padding="same")(m_cat2)
  m_act8 = LeakyReLU(0.2)(m_conv8)
  # (720, 1280, 32)
  m_cat3 = Concatenate()([m_act8, m_act0])
  m_conv9 = Conv2DTranspose(3, kernel_size=3, strides=1, padding="same")(m_cat3)
  m_act9 = Activation("sigmoid")(m_conv9)
  # (720, 1280, 3)
  m_out = Rescaling(scale=255.0)(m_act9)
  return Model(m_in, m_out)


def main():
  # model = make_model()
  model = keras.api.saving.load_model("model.keras")
  model.compile("adam", "mse")
  generator = av.Generator("Fast.and.the.Furious.Tokyo.Drift.mp4", (1280, 720), 16)
  running = True
  while running:
    for i in range(50):
      x, y = generator.generate_batch()
      if x is None:
        print("end of stream ", i)
        running = False
        break
      x = x[:, :, :, :3]
      y = y[:, :, :, :3]
      logs = model.train_on_batch(x, y)
      print(logs)
    model.save("model.keras", overwrite=True)
    print("Model saved")


if __name__ == "__main__":
  main()
