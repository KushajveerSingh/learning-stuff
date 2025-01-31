import styles from './Footer.module.css';
import { FiFacebook, FiTwitter, FiInstagram } from 'react-icons/fi';
import { FooterOverlay, Newsletter } from '../../components';
const Footer = () => {
  return (
    <div className={`${styles.app__footer} section__padding`}>
      <FooterOverlay />
      <Newsletter />

      <div className={styles.app__footer_links}>
        <div className={styles.app__footer_links_contact}>
          <h1 className={styles.app__footer_headtext}>Contact Us</h1>
          <p className="p__opensans">9 W 53rd St, New York, NY 10019, USA</p>
          <p className="p__opensans">+1 212-344-1230</p>
          <p className="p__opensans">+1 212-555-1230</p>
        </div>

        <div className={styles.app__footer_links_logo}>
          <img src="/gericht.png" alt="footer logo" />
          <p className="p__opensans">
            &quot;The best way to find yourself is to lose yourself in the service of
            others.&quot;
          </p>
          <img
            src="/spoon.png"
            alt="spoon"
            className="spoon_img"
            style={{ marginTop: '15px' }}
          />

          <div className={styles.app__footer_links_icons}>
            <FiFacebook />
            <FiTwitter />
            <FiInstagram />
          </div>
        </div>

        <div className={styles.app__footer_links_work}>
          <h1 className={styles.app__footer_headtext}>Working Hours</h1>
          <p className="p__opensans">Monday-Friday:</p>
          <p className="p__opensans">08:00 am - 12:00 am</p>
          <p className="p__opensans">Saturday-Sunday:</p>
          <p className="p__opensans">07:00 am - 11:00 pm</p>
        </div>
      </div>

      <div className={styles.footer__copyright}>
        <p className="p__opensans">2022 Gericht. All Rights reserved</p>
      </div>
    </div>
  );
};

export default Footer;
