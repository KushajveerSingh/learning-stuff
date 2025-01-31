import type { NextPage } from 'next';

import {
  Navbar,
  Header,
  AboutUs,
  SpecialMenu,
  Chef,
  Intro,
  Laurels,
  Gallery,
  FindUs,
  Footer,
} from '../containers';
import { MetaTags } from '../components';

const Home: NextPage = () => {
  return (
    <div>
      <MetaTags
        title="Home - Gericht"
        description="Converted Figma design into fully-responsive Nextjs/TypeScript/CSS(BEM) web app and deployed on Vercel"
        image="/demo_image.png"
      />

      <Navbar />
      <Header />
      <AboutUs />
      <SpecialMenu />
      <Chef />
      <Intro />
      <Laurels />
      <Gallery />
      <FindUs />
      <Footer />
    </div>
  );
};

export default Home;
