import styles from './Gallery.module.css';
import { useRef } from 'react';
import { BsInstagram, BsArrowLeftShort, BsArrowRightShort } from 'react-icons/bs';
import { SubHeading } from '../../components';

const images = ['/gallery01.png', '/gallery02.png', '/gallery03.png', '/gallery04.png'];

interface RefProps extends HTMLDivElement {
  scrollLeft: number;
}

const Gallery = () => {
  const scrollRef = useRef<RefProps>(null);

  const scroll = (direction: string) => {
    const { current } = scrollRef;

    if (current !== null) {
      if (direction === 'left') {
        current.scrollLeft -= 300;
      } else {
        current.scrollLeft += 300;
      }
    }
  };

  return (
    <div className={`${styles.app__gallery} flex__center`}>
      <div className={styles.app__gallery_content}>
        <SubHeading title="Instagram" />

        <h1 className="headtext__cormorant">Photo Gallery</h1>
        <p className="p__opensans" style={{ color: '#AAA', marginTop: '2rem' }}>
          Drawing inspiration from classic Americana, we reinvent the neighborhood eatery
          to create a timeless fusion of old and new.
        </p>

        <button type="button" className="custom__button">
          View More
        </button>
      </div>

      <div className={styles.app__gallery_images}>
        <div className={styles.app__gallery_images_container} ref={scrollRef}>
          {images.map((image, index) => (
            <div
              className={`${styles.app__gallery_images_card} flex__center`}
              key={`gallery_image_${index}`}
            >
              <img src={image} alt="gallery" />
              <BsInstagram className={styles.gallery__image_icon} />
            </div>
          ))}
        </div>

        <div className={styles.app__gallery_images_arrows}>
          <BsArrowLeftShort
            className={styles.gallery__arrow_icon}
            onClick={() => scroll('left')}
          />
          <BsArrowRightShort
            className={styles.gallery__arrow_icon}
            onClick={() => scroll('right')}
          />
        </div>
      </div>
    </div>
  );
};

export default Gallery;
